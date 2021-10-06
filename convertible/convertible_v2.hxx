#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
    namespace concepts
    {
        namespace cpp20
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
    }
}

#undef FWD