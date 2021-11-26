#pragma once

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

// Workaround unimplemented concepts & type traits (specifically with libc++)
// NOTE: Intentially placed in 'std' to detect (compiler error) when implemented (backported?)
//       since below definitions aren't as conforming as std equivalents).
#if defined(__clang__) && defined(_LIBCPP_VERSION) // libc++
namespace std
{
#if (__clang_major__ <= 13 && (defined(__APPLE__) || defined(__EMSCRIPTEN__))) || __clang_major__ < 13
    // Credit: https://en.cppreference.com

    template< class lhs_t, class rhs_t >
    concept assignable_from =
        std::is_lvalue_reference_v<lhs_t> &&
        requires(lhs_t lhs, rhs_t && rhs) {
            { lhs = std::forward<rhs_t>(rhs) } -> std::same_as<lhs_t>;
        };

    template <class from_t, class to_t>
    concept convertible_to =
        std::is_convertible_v<from_t, to_t> &&
        requires { static_cast<to_t>(std::declval<from_t>()); };

    template<class T, class U>
    concept equality_comparable_with =
        requires(const std::remove_reference_t<T>&t,
            const std::remove_reference_t<U>&u) {
                { t == u } -> convertible_to<bool>;
                { t != u } -> convertible_to<bool>;
                { u == t } -> convertible_to<bool>;
                { u != t } -> convertible_to<bool>;
        };

    template<class T>
    concept copy_constructible =
        std::is_move_constructible_v<T> &&
        std::is_constructible_v<T, T&> && std::convertible_to<T&, T> &&
        std::is_constructible_v<T, const T&> && std::convertible_to<const T&, T> &&
        std::is_constructible_v<T, const T> && std::convertible_to<const T, T>;

    template<class F, class... Args>
    concept invocable =
        requires(F&& f, Args&&... args) {
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            /* not required to be equality preserving */
        };
    
    template< class lhs_t, class rhs_t >
    using common_reference_t = std::enable_if_t<std::convertible_to<lhs_t, rhs_t> && std::convertible_to<rhs_t, lhs_t>>;
#endif
}
#endif

namespace std
{
    template<typename rhs_t, typename lhs_t>
    concept assignable_to = std::assignable_from<lhs_t, rhs_t>;
}

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

        namespace readers
        {
            struct identity
            {
                decltype(auto) operator()(auto&& obj) const
                {
                    return FWD(obj);
                }
            };

            template<concepts::member_ptr member_ptr_t>
            struct member
            {
                using class_t = traits::member_class_t<member_ptr_t>;

                member(member_ptr_t ptr):
                    ptr_(ptr)
                {}

                decltype(auto) operator()(std::convertible_to<class_t> auto&& obj) const
                {
                    if constexpr(std::is_rvalue_reference_v<decltype(obj)>)
                    {
                        return std::move(obj.*ptr_);
                    }
                    else
                    {
                        return obj.*ptr_;
                    }
                }

                member_ptr_t ptr_;
            };
        }

        template<typename obj_t = details::placeholder, typename reader_t = readers::identity>
            // Workaround: Clang doesn't approve it in template parameter declaration.
            requires std::copy_constructible<reader_t>
        struct object
        {
            
            using out_t = std::invoke_result_t<reader_t, obj_t>;

            auto create(auto&& obj) const
                requires std::invocable<reader_t, decltype(obj)>
            {
                return object<decltype(obj), reader_t>(FWD(obj), reader_);
            }

            object() = default;
            object(const object&) = default;
            object(object&&) = default;
            explicit object(std::convertible_to<obj_t> auto&& obj)
                requires std::invocable<reader_t, decltype(obj)>
                : obj_(FWD(obj))
            {
            }

            explicit object(std::convertible_to<obj_t> auto&& obj, std::invocable<obj_t> auto&& reader)
                : obj_(FWD(obj)), reader_(FWD(reader))
            {
            }

            operator out_t() const
            {
                return static_cast<out_t>(reader_(obj_));
            }

            decltype(auto) operator=(const object& other)
            {
                return *this = static_cast<std::decay_t<out_t>>(other);
            }

            decltype(auto) operator=(std::assignable_to<out_t&> auto&& val)
            {
                reader_(obj_) = FWD(val);
                return *this;
            }

            decltype(auto) operator==(const std::equality_comparable_with<out_t> auto& val) const
            {
                return reader_(obj_) == val;
            }

            obj_t obj_;
            reader_t reader_;
        };

        // Workaround: MSVC does not like auto parameters in deduction guides.

        template<typename obj_t>
        object(obj_t& obj)->object<obj_t&>;
        
        template<typename obj_t>
        object(obj_t&& obj)->object<obj_t&&>;

        template<typename obj_t, typename reader_t>
            requires std::invocable<reader_t, obj_t>
        object(obj_t& obj, reader_t&& reader)->object<obj_t&, std::remove_reference_t<reader_t>>;

        template<typename obj_t, typename reader_t>
            requires std::invocable<reader_t, obj_t>
        object(obj_t&& obj, reader_t&& reader)->object<obj_t&&, std::remove_reference_t<reader_t>>;

        template<concepts::member_ptr member_ptr_t>
        auto member(member_ptr_t&& ptr, auto&& obj)
        {
            return object<decltype(obj), readers::member<member_ptr_t>>(FWD(obj), ptr);
        }

        template<concepts::member_ptr member_ptr_t>
        auto member(member_ptr_t&& ptr)
        {
            constexpr traits::member_class_t<member_ptr_t>* garbage = nullptr;
            return object<traits::member_class_t<member_ptr_t>&, readers::member<member_ptr_t>>(*garbage, ptr);
        }
    }

    namespace operators
    {

        struct assign
        {
            template<typename lhs_t> // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
            decltype(auto) exec(lhs_t&& lhs, std::assignable_to<lhs_t> auto&& rhs) const
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
        void assign(concepts::adaptable<lhs_adapter_t> auto&& lhs, concepts::adaptable<rhs_adapter_t> auto&& rhs) const
        {
            constexpr operators::assign op;

            auto lhsAdap = lhsAdapter_.create(FWD(lhs));
            auto rhsAdap = rhsAdapter_.create(FWD(rhs));
            
            if constexpr(dir == direction::rhs_to_lhs)
                op.exec(lhsAdap, converter_(std::move(rhsAdap)));
            else
                op.exec(rhsAdap, converter_(std::move(lhsAdap)));
        }

        bool equal(const concepts::adaptable<lhs_adapter_t> auto& lhs, const concepts::adaptable<rhs_adapter_t> auto& rhs) const
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
        void assign(auto&& lhs, auto&& rhs) const
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
        `std::common_type<object<...>, object<...>>`
    Specifically enables:
        `std::assignable_from<object<...>, object<...>>`
    */
    template<typename... a_ts, typename... b_ts>
    struct common_type<convertible::adapters::object<a_ts...>, convertible::adapters::object<b_ts...>>
    {
        using type = ::std::common_reference_t<
            typename convertible::adapters::object<a_ts...>::out_t,
            typename convertible::adapters::object<b_ts...>::out_t
        >;
    };
}

#undef FWD
#undef MSVC_ENUM_FIX
