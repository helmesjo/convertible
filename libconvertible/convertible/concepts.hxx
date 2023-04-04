#pragma once

#include <convertible/common.hxx>
#include <convertible/std_concepts_compat.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <concepts>
#include <span>
#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
  template<typename obj_t, typename reader_t>
  struct adapter;

  namespace traits
  {
    namespace details
    {
      template<typename... arg_ts>
      struct is_adapter: std::false_type {};

      template<typename... arg_ts>
      struct is_adapter<adapter<arg_ts...>>: std::true_type {};
    }

    template<typename T>
    constexpr bool is_adapter_v = details::is_adapter<std::remove_cvref_t<T>>::value;

    template<direction dir, typename arg1_t, typename arg2_t>
    using lhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg1_t, arg2_t>;

    template<direction dir, typename arg1_t, typename arg2_t>
    using rhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg2_t, arg1_t>;

    template<typename converter_t, typename arg_t>
    using converted_t = std::invoke_result_t<converter_t, arg_t>;
  }

  namespace concepts
  {
    template<typename adaptee_t, typename reader_t>
    concept adaptable = requires(reader_t&& reader, adaptee_t&& adaptee)
    {
      { FWD(reader)(FWD(adaptee)) };
      requires (!std::same_as<void, decltype(FWD(reader)(FWD(adaptee)))>);
    };

    template<typename T>
    concept adapter = traits::is_adapter_v<T>;

    template<typename adapter_t>
    concept adaptee_type_known = (adapter<adapter_t> && !std::same_as<typename adapter_t::adaptee_t, details::any>);

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable = requires(mapping_t&& map)
    {
      requires (dir == direction::rhs_to_lhs
                && requires { { FWD(map)(std::declval<rhs_t>()) }; })
                || requires { { FWD(map)(std::declval<lhs_t>()) }; };
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable_assign = requires(mapping_t&& map, lhs_t&& lhs, rhs_t&& rhs)
    {
      FWD(map).template assign<dir>(FWD(lhs), FWD(rhs));
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable_equal = requires(mapping_t&& map, lhs_t&& lhs, rhs_t&& rhs)
    {
      FWD(map).template equal<dir>(FWD(lhs), FWD(rhs));
    };

    template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
    concept assignable_from_converted = requires(traits::lhs_t<dir, lhs_t, rhs_t> lhs, traits::converted_t<converter_t, traits::rhs_t<dir, lhs_t, rhs_t>> rhs)
    {
      { lhs = rhs };
    };

    template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
    concept equality_comparable_with_converted = requires(traits::lhs_t<dir, lhs_t, rhs_t>&& lhs, traits::converted_t<converter_t, traits::rhs_t<dir, lhs_t, rhs_t>>&& rhs)
    {
      { FWD(lhs) == FWD(rhs) } -> std::convertible_to<bool>;
    };
  }

  template<concepts::adapter _lhs_adapter_t, concepts::adapter _rhs_adapter_t, typename _converter_t>
  struct mapping;

  namespace traits
  {
    namespace details
    {
      template<typename... arg_ts>
      struct is_mapping: std::false_type {};

      template<typename... arg_ts>
      struct is_mapping<mapping<arg_ts...>>: std::true_type {};
    }

    template<typename T>
    constexpr bool is_mapping_v = details::is_mapping<std::remove_cvref_t<T>>::value;

    template<typename arg_t, concepts::adapter... adapter_ts>
    constexpr std::size_t adaptable_count_v = (concepts::adaptable<arg_t, adapter_ts> +...);

    template<concepts::adapter adapter_t, typename adaptee_t>
    using adapted_t = converted_t<adapter_t, adaptee_t>;
  }

  namespace concepts
  {
    template<typename T>
    concept mapping = traits::is_mapping_v<T>;
  }

  template<concepts::mapping... mapping_ts>
  struct mapping_table;

  namespace traits::details
  {
      template<typename... arg_ts>
      struct is_mapping<mapping_table<arg_ts...>>: std::true_type {};
  }
}

#undef FWD
