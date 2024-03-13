#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>

namespace convertible
{
  namespace details
  {
    struct any
    {
      constexpr any& operator[](std::size_t)
      {
        return *this;
      }

      constexpr any& operator*()
      {
        return *this;
      }
    };

    template<typename value_t>
    struct const_value
    {
      constexpr const_value() = default;
      constexpr const_value(value_t val)
        requires std::copy_constructible<value_t>
      : value(val){}

      constexpr const_value(auto&& str)
        requires (!std::copy_constructible<value_t>) &&
        requires { std::begin(str); std::size(str); }
      {
          std::copy_n(std::begin(str), std::size(str), value);
      }

      template<typename to_t>
      operator to_t() const
        requires std::common_reference_with<to_t, value_t>
      {
        return value;
      }

      value_t value;
    };
    template<typename elem_t, std::size_t size>
    const_value(const elem_t (&str)[size]) -> const_value<elem_t[size]>; //NOLINT
  }

  enum class direction
  {
    lhs_to_rhs,
    rhs_to_lhs
  };
}
