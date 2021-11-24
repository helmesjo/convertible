#pragma once

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
// Workaround for MSVC bug: https://developercommunity2.visualstudio.com/t/Concept-satisfaction-failure-for-member/1489332
#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define MSVC_ENUM_FIX(...) int

    namespace direction
    {
        static constexpr auto lhs_to_rhs = 0;
        static constexpr auto rhs_to_lhs = 1;
    };
#else
#define MSVC_ENUM_FIX(...) __VA_ARGS__
    enum class direction
    {
        lhs_to_rhs,
        rhs_to_lhs
    };
#endif

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
        concept class_type = std::is_class_v<T>;

        template<typename T>
        concept member_ptr = std::is_member_pointer_v<std::decay_t<T>>;

        template<typename rhs_t, typename lhs_t>
        concept assignable_to = std::assignable_from<lhs_t, rhs_t>;

        template<typename arg_t, typename adapter_t>
        concept adaptable = requires(adapter_t adapter, arg_t arg)
        {
            { adapter.create(arg) } -> class_type;
        };

        template<typename mapping_t, typename lhs_t, typename rhs_t>
        concept mappable = requires(mapping_t mapping, lhs_t lhs, rhs_t rhs)
        {
            mapping.template assign<direction::rhs_to_lhs>(lhs, rhs);
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
            object(const object&) = default;
            object(object&&) = default;
            explicit object(std::convertible_to<obj_t> auto&& obj): obj_(FWD(obj))
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
                return *this = static_cast<std::decay_t<obj_t>>(other);
            }

            decltype(auto) operator=(concepts::assignable_to<obj_t&> auto&& val)
            {
                obj_ = val;
                return *this;
            }

            decltype(auto) operator==(const std::equality_comparable_with<obj_t> auto& val) const
            {
                return obj_ == val;
            }

            obj_t obj_;
        };

        template<typename obj_t>
        object(obj_t&)->object<obj_t&>;

        template<typename obj_t>
        object(obj_t&&)->object<obj_t&&>;

        template<concepts::member_ptr member_ptr_t, bool is_rval = false>
        struct member
        {
            using instance_t = traits::member_class_t<member_ptr_t>;
            using value_t = traits::member_value_t<member_ptr_t>;

            std::decay_t<member_ptr_t> ptr_;
            instance_t* inst_;

            auto create(std::convertible_to<instance_t> auto&& obj) const
            {
                return member<decltype(ptr_), std::is_rvalue_reference_v<decltype(obj)>>(ptr_, FWD(obj));
            }

            member() = default;
            member(const member&) = default;
            member(member&&) = default;
            explicit member(concepts::member_ptr auto&& ptr): ptr_(ptr){}
            explicit member(concepts::member_ptr auto&& ptr, std::convertible_to<instance_t> auto&& inst): ptr_(ptr), inst_(&inst)
            {
            }

            operator decltype(auto)() const
            {
                if constexpr(is_rval)
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

            decltype(auto) operator=(concepts::assignable_to<value_t&> auto&& val)
            {
                inst_->*ptr_ = FWD(val);
                return *this;
            }

            decltype(auto) operator==(const std::equality_comparable_with<value_t> auto& val) const
            {
                return inst_->*ptr_ == val;
            }
        };

        template<concepts::member_ptr member_ptr_t>
        member(member_ptr_t ptr)->member<member_ptr_t, false>;

        template<concepts::member_ptr member_ptr_t, typename instance_t>
        member(member_ptr_t ptr, instance_t& inst)->member<member_ptr_t, false>;

        template<concepts::member_ptr member_ptr_t, typename instance_t>
        member(member_ptr_t ptr, instance_t&& inst)->member<member_ptr_t, true>;
    }

    namespace operators
    {

        struct assign
        {
            template<typename lhs_t> // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
            decltype(auto) exec(lhs_t&& lhs, concepts::assignable_to<lhs_t> auto&& rhs) const
            {
                return lhs = FWD(rhs);
            }
        };

        struct equal
        {
            template<typename lhs_t> // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
            decltype(auto) exec(const lhs_t& lhs, const std::equality_comparable_with<lhs_t> auto& rhs) const
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

    template<typename lhs_adapter_t, typename rhs_adapter_t, typename converter_t = converters::identity>
    struct mapping
    {
        explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter = {}):
            lhsAdapter_(std::move(lhsAdapter)),
            rhsAdapter_(std::move(rhsAdapter)),
            converter_(converter)
        {}

        template<MSVC_ENUM_FIX(direction) dir>
        decltype(auto) assign(concepts::adaptable<lhs_adapter_t> auto&& lhs, concepts::adaptable<rhs_adapter_t> auto&& rhs) const
        {
            constexpr operators::assign op;

            auto lhsAdap = lhsAdapter_.create(FWD(lhs));
            auto rhsAdap = rhsAdapter_.create(FWD(rhs));
            
            if constexpr(dir == direction::rhs_to_lhs)
                return op.exec(lhsAdap, converter_(std::move(rhsAdap)));
            else
                return op.exec(rhsAdap, converter_(std::move(lhsAdap)));
        }

        decltype(auto) equal(const concepts::adaptable<lhs_adapter_t> auto& lhs, const concepts::adaptable<rhs_adapter_t> auto& rhs) const
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

        template<MSVC_ENUM_FIX(direction) dir>
        decltype(auto) assign(auto&& lhs, auto&& rhs) const
        {
            using lhs_t = decltype(lhs);
            using rhs_t = decltype(rhs);

            for_each([&](auto&& map){
                using mapping_t = decltype(map);

                if constexpr(concepts::mappable<mapping_t, lhs_t, rhs_t>)
                {
                    map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
                }
            }, mappings_);
        }

        decltype(auto) equal(auto&& lhs, auto&& rhs) const
        {
        }

        std::tuple<mapping_ts...> mappings_;
    };
}

namespace std
{
    /* 
    Specialization for:
        `std::common_type<object<A>, object<B>>`
    Specifically enables:
        `std::assignable_from<object<A>, object<B>>`
        where `common_type_t<A, B>` exists.
    */
    template<typename A, typename B>
    struct common_type<convertible::adapters::object<A>, convertible::adapters::object<B>>
    {
        using type = std::common_reference_t<A, B>;
    };

    /* 
    Specialization for:
        `std::common_type<member<A>, member<B>>`
    Specifically enables:
        `std::assignable_from<member<A>, member<B>>`
        where `common_type_t<A, B>` exists.
    */
    template<typename A1, bool A2, typename B1, bool B2>
    struct common_type<convertible::adapters::member<A1, A2>, convertible::adapters::member<B1, B2>>
    {
        using type = std::common_reference_t<convertible::traits::member_value_t<A1>, convertible::traits::member_value_t<B1>>;
    };
}

#undef FWD
#undef MSVC_ENUM_FIX
