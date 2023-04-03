#pragma once

#include <type_traits>
#include <utility>
#include <version>

// Workarounds for unimplemented concepts & type traits (specifically with libc++)
// NOTE: Intentionally placed in 'std' to be easily removed when no longer needed
//       since below definitions aren't as conforming as std equivalents).
#if defined(__clang__) && defined(_LIBCPP_VERSION) // libc++

namespace std
{
#if ((__clang_major__ < 13 || (__clang_major__ == 13 && __clang_minor__ == 0)) && (defined(__APPLE__) || defined(__EMSCRIPTEN__))) || __clang_major__ < 13
  // Credit: https://en.cppreference.com

  template<class lhs_t, class rhs_t>
  concept assignable_from =
    std::is_lvalue_reference_v<lhs_t> &&
    requires(lhs_t lhs, rhs_t && rhs) {
      { lhs = std::forward<rhs_t>(rhs) } -> std::same_as<lhs_t>;
    };

  template<class Derived, class Base>
  concept derived_from =
    std::is_base_of_v<Base, Derived> &&
    std::is_convertible_v<const volatile Derived*, const volatile Base*>;

  template<class from_t, class to_t>
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

  template<class F, class... Args>
  concept invocable =
    requires(F&& f, Args&&... args) {
      std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
      /* not required to be equality preserving */
    };
#endif
}
#endif

#if !defined(__cpp_lib_forward_like)
namespace std
{
  // Credit: https://en.cppreference.com/w/cpp/utility/forward_like
  template<class T, class U>
  [[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept
  {
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>)
    {
      if constexpr (is_adding_const)
        return std::as_const(x);
      else
        return static_cast<U&>(x);
    }
    else
    {
      if constexpr (is_adding_const)
        return std::move(std::as_const(x));
      else
        return std::move(x);
    }
  }
}
#endif