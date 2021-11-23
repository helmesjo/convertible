#pragma once

#include <concepts>
#include <tuple>
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

    namespace concepts
    {
        template<typename T>
        concept member_ptr = std::is_member_pointer_v<std::decay_t<T>>;
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
            object(const object&) = default;
            object(object&&) = default;
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

        template<concepts::member_ptr member_ptr_t, typename instance_t>
        struct member
        {
            using value_t = traits::member_value_t<member_ptr_t>;

            std::decay_t<member_ptr_t> ptr_;
            std::decay_t<instance_t>* inst_;

            auto create(auto&& obj) const
            {
                return member<decltype(ptr_), decltype(obj)>(ptr_, FWD(obj));
            }

            explicit member(concepts::member_ptr auto&& ptr): ptr_(ptr){}
            explicit member(concepts::member_ptr auto&& ptr, auto&& inst): ptr_(ptr), inst_(&inst)
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

        template<concepts::member_ptr member_ptr_t>
        member(member_ptr_t ptr)->member<member_ptr_t, traits::member_class_t<member_ptr_t>&>;

        template<concepts::member_ptr member_ptr_t, typename instance_t>
        member(member_ptr_t ptr, instance_t& inst)->member<member_ptr_t, instance_t&>;

        template<concepts::member_ptr member_ptr_t, typename instance_t>
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

    namespace converters
    {
        struct identity
        {
            constexpr decltype(auto) operator()(auto&& val) const
            {
                return FWD(val);
            }
        };
    }

    enum class direction
    {
        lhs_to_rhs,
        rhs_to_lhs
    };

    template<typename lhs_adapter_t, typename rhs_adapter_t, typename converter_t = converters::identity>
    struct mapping
    {
        explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter = {}):
            lhsAdapter_(std::move(lhsAdapter)),
            rhsAdapter_(std::move(rhsAdapter)),
            converter_(converter)
        {}

        template<direction dir>
        decltype(auto) assign(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::assign op;
            if constexpr(dir == direction::rhs_to_lhs)
                return op.exec(lhsAdapter_.create(FWD(lhs)), converter_(rhsAdapter_.create(FWD(rhs))));
            else
                return op.exec(rhsAdapter_.create(FWD(rhs)), converter_(lhsAdapter_.create(FWD(lhs))));
        }

        decltype(auto) equal(auto&& lhs, auto&& rhs) const
        {
            constexpr operators::equal op;
            return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
        }

        std::decay_t<lhs_adapter_t> lhsAdapter_;
        std::decay_t<rhs_adapter_t> rhsAdapter_;
        converter_t converter_;
    };

    template <std::size_t begin, std::size_t end, typename callback_t, typename... arg_ts>
    constexpr void for_each(callback_t callback, arg_ts... args)
    {
        if constexpr (begin < end)
        {
            callback(std::get<begin>(std::tuple{FWD(args)...}));
            for_each<begin + 1, end>(callback, FWD(args)...);
        }
    }

    template <typename callback_t, typename... arg_ts>
    constexpr void for_each(callback_t callback, arg_ts... args)
    {
        for_each<0, sizeof...(args)>(callback, FWD(args)...);
    }

    template <typename callback_t, typename... arg_ts>
    constexpr void for_each(callback_t callback, std::tuple<arg_ts...> args)
    {
        std::apply([&](auto&&... args){
            for_each<0, sizeof...(args)>(callback, FWD(args)...);
        }, args);
    }

    template<typename... mapping_ts>
    struct mapping_table
    {
        mapping_table(mapping_ts... mappings):
            mappings_(mappings...)
        {
        }

        template<direction dir>
        decltype(auto) assign(auto&& lhs, auto&& rhs) const
        {
            for_each([&](auto&& map){
                using mapping_t = decltype(map);
                map.template assign<dir>(FWD(lhs), FWD(rhs));
            }, mappings_);
        }

        decltype(auto) equal(auto&& lhs, auto&& rhs) const
        {
        }

        std::tuple<mapping_ts...> mappings_;
    };
}

#undef FWD