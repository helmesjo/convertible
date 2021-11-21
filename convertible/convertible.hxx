#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
    namespace traits
    {
        namespace details
        {
            template<typename M>
            struct member_ptr_meta
            {
                template<typename R>
                struct return_t{ using type = R; };

                template <typename C, typename R>
                static return_t<C> get_class_type(R C::*);

                template <typename C, typename R>
                static return_t<R> get_value_type(R C::*);

                using class_t = typename decltype(get_class_type(std::declval<M>()))::type;
                using value_t = typename decltype(get_value_type(std::declval<M>()))::type;
            };
        }

        template<typename member_ptr_t>
        using member_class_t = typename details::member_ptr_meta<member_ptr_t>::class_t;
        
        template<typename member_ptr_t, typename class_t = member_class_t<member_ptr_t>, typename value_t = typename details::member_ptr_meta<member_ptr_t>::value_t>
        using member_value_t = std::conditional_t<std::is_rvalue_reference_v<class_t>, value_t&&, value_t>;
    }

    namespace concepts
    {
        template<typename T>
        concept iterable = requires(T t){ std::begin(std::declval<T&>()) == std::end(std::declval<T&>()); };

        template<typename T>
        concept dereferenceable = requires(T t){ *t; };

        template <typename lhs_t, typename rhs_t, typename converter_t>
        concept assignable = requires(lhs_t lhs, rhs_t rhs, converter_t converter){ {lhs = converter(rhs)}; };

        template <typename lhs_t, typename rhs_t, typename converter_t>
        concept comparable = requires(lhs_t lhs, rhs_t rhs, converter_t converter){ {lhs == converter(rhs)}; };

        template<typename T, typename U>
        concept adaptable = requires(T t, U u)
        {
            //requires std::is_constructible_v<T, std::decay_t<U>&>;
            requires std::is_convertible_v<T, std::remove_reference_t<U>>;
            { t.operator=(u) };
            { t.operator==(u) } -> std::convertible_to<bool>;
        };

        template<typename adapter1_t, typename arg1_t, typename adapter2_t, typename arg2_t>
        concept mappable = requires(adapter1_t adpt1, arg1_t arg1, adapter2_t adpt2, arg2_t arg2)
        {
            requires adaptable<adapter1_t, arg1_t>;
            requires adaptable<adapter2_t, arg2_t>;
            requires std::convertible_to<arg1_t, arg2_t>;
        };
    }

    namespace adapters
    {
        namespace details
        {
            struct placeholder{};
        }

        template<typename obj_t = details::placeholder>
        struct object
        {
            auto create(auto&& obj) const
            {
                return object<decltype(obj)>{FWD(obj)};
            }

            object() = default;
            explicit object(obj_t obj): obj_(FWD(obj))
            {
            }

            operator decltype(auto)() const
            {
                if constexpr(std::is_rvalue_reference_v<obj_t>)
                {
                    return std::move(obj_);
                }
                else
                {
                    return obj_;
                }
            }

            auto operator=(const object& other)
            {
                return obj_ = other;
            }

            auto operator=(auto&& val)
            {
                return obj_ = val;
            }

            auto operator==(const auto& val) const
            {
                return obj_ == val;
            }

            obj_t obj_;
        };

        template<typename obj_t>
        object(obj_t&)->object<obj_t&>;

        template<typename obj_t>
        object(obj_t&&)->object<obj_t&&>;

        template<typename member_ptr_t, typename instance_t = traits::member_class_t<member_ptr_t>>
            requires std::is_member_pointer_v<member_ptr_t>
        struct member
        {
            using value_t = traits::member_value_t<member_ptr_t>;

            member_ptr_t ptr_;
            std::decay_t<instance_t>* inst_;

            auto create(auto&& obj) const
            {
                return member<member_ptr_t, decltype(obj)>(ptr_, FWD(obj));
            }

            explicit member(member_ptr_t ptr): ptr_(ptr){}
            explicit member(member_ptr_t ptr, instance_t inst): ptr_(ptr), inst_(&inst)
            {
            }

            operator decltype(auto)() const
            {
                if constexpr(std::is_rvalue_reference_v<instance_t>)
                {
                    return std::move(inst_->*ptr_);
                }
                else
                {
                    return inst_->*ptr_;
                }
            }

            auto operator=(const member& other)
            {
                return *this = static_cast<value_t>(other);
            }

            auto operator=(auto&& val)
            {
                return inst_->*ptr_ = FWD(val);
            }

            auto operator==(const auto& val) const
            {
                return inst_->*ptr_ == val;
            }
        };

        template<typename member_ptr_t, typename instance_t>
        member(member_ptr_t ptr, instance_t& inst)->member<member_ptr_t, instance_t&>;

        template<typename member_ptr_t, typename instance_t>
        member(member_ptr_t ptr, instance_t&& inst)->member<member_ptr_t, instance_t&&>;
    }

    namespace operators
    {
        struct assign
        {
            decltype(auto) exec(auto&& lhs, auto&& rhs) const
            {
                return FWD(lhs) = FWD(rhs);
            }
        };

        struct equal
        {
            decltype(auto) exec(auto&& lhs, auto&& rhs) const
            {
                return FWD(lhs) == FWD(rhs);
            }
        };
    }

    enum class direction
    {
        lhs_to_rhs,
        rhs_to_lhs
    };

    template<typename lhs_adapter_t, typename rhs_adapter_t>
    struct mapping
    {
        explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter):
            lhsAdapter_(std::move(lhsAdapter)),
            rhsAdapter_(std::move(rhsAdapter))
        {}

        template<direction dir>
        auto assign(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::assign op;
            if constexpr(dir == direction::rhs_to_lhs)
                return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
            else
                return op.exec(rhsAdapter_.create(FWD(rhs)), lhsAdapter_.create(FWD(lhs)));
        }

        auto equal(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::equal op;
            return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
        }

        std::decay_t<lhs_adapter_t> lhsAdapter_;
        std::decay_t<rhs_adapter_t> rhsAdapter_;
    };
}

#undef FWD