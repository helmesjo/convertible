#pragma once

#include <convertible/concepts.hxx>
#include <convertible/std_concepts_ext.hxx>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::converter
{
  struct identity
  {
    constexpr decltype(auto) operator()(auto&& val) const
    {
      return FWD(val);
    }
  };

  template<typename to_t, typename converter_t>
  struct explicit_cast
  {
    constexpr explicit_cast(converter_t& converter):
      converter_(converter)
    {}

    template<typename arg_t>
    using converted_t = traits::converted_t<converter_t, arg_t>;

    constexpr decltype(auto) operator()(auto&& obj) const
      requires std::is_assignable_v<to_t&, converted_t<decltype(obj)>>
            || std_ext::castable_to<converted_t<decltype(obj)>, to_t>
    {
      if constexpr(std::is_assignable_v<to_t&, converted_t<decltype(obj)>>)
      {
        return converter_(FWD(obj));
      }
      else
      {
        return static_cast<to_t>(converter_(FWD(obj)));
      }
    }

    converter_t& converter_;
  };
}

#undef FWD
