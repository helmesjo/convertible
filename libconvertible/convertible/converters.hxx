#pragma once

#include <convertible/concepts.hxx>
#include <convertible/std_concepts_ext.hxx>
#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::converter
{
  struct identity
  {
    constexpr auto operator()(auto&& val) const -> decltype(auto)
    {
      return FWD(val);
    }
  };

  template<typename target_t, typename converter_t>
  struct explicit_cast
  {
    constexpr explicit_cast(converter_t& converter):
      converter_(converter)
    {}

    template<typename arg_t>
    using converted_t = traits::converted_t<converter_t, arg_t>;

    constexpr auto operator()(auto&& obj) const -> decltype(auto)
      requires std::common_reference_with<std::remove_cvref_t<target_t>, converted_t<decltype(obj)>>
            || concepts::castable_to<converted_t<decltype(obj)>, target_t>
    {
      if constexpr(std::common_reference_with<std::remove_cvref_t<target_t>, converted_t<decltype(obj)>>)
      {
        return converter_(FWD(obj));
      }
      else
      {
        return static_cast<target_t>(converter_(FWD(obj)));
      }
    }

    converter_t& converter_; //NOLINT
  };
}

#undef FWD
