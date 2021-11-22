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
        using member_value_t = value_t;
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
            explicit object(auto&& obj): obj_(FWD(obj))
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

            decltype(auto) operator=(const object& other)
            {
                return obj_ = other;
            }

            decltype(auto) operator=(auto&& val)
            {
                return obj_ = val;
            }

            decltype(auto) operator==(const auto& val) const
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

            std::decay_t<member_ptr_t> ptr_;
            std::decay_t<instance_t>* inst_;

            auto create(auto&& obj) const
            {
                return member<member_ptr_t, decltype(obj)>(ptr_, FWD(obj));
            }

            explicit member(auto&& ptr): ptr_(ptr){}
            explicit member(auto&& ptr, auto&& inst): ptr_(ptr), inst_(&inst)
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

            decltype(auto) operator=(const member& other)
            {
                return *this = static_cast<value_t>(other);
            }

            decltype(auto) operator=(auto&& val)
            {
                return inst_->*ptr_ = FWD(val);
            }

            decltype(auto) operator==(const auto& val) const
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
        decltype(auto) assign(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::assign op;
            if constexpr(dir == direction::rhs_to_lhs)
                return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
            else
                return op.exec(rhsAdapter_.create(FWD(rhs)), lhsAdapter_.create(FWD(lhs)));
        }

        decltype(auto) equal(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::equal op;
            return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
        }

        std::decay_t<lhs_adapter_t> lhsAdapter_;
        std::decay_t<rhs_adapter_t> rhsAdapter_;
    };
}

#undef FWD