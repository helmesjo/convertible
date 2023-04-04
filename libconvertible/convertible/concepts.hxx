#pragma once

#include <convertible/common.hxx>
#include <convertible/std_concepts_compat.hxx>

#include <concepts>
#include <span>
#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

// std extended
namespace std_ext
{
  template<typename as_t, typename with_t>
  using like_t = decltype(std::forward_like<as_t>(std::declval<with_t>()));
}

namespace convertible
{
  template<typename obj_t, typename reader_t>
  struct adapter;

  namespace traits
  {
    namespace details
    {
      template<typename C, typename R, typename... Args>
      struct member_meta
      {
        using class_t = C;
        using value_t = R;
      };
      template<typename class_t, typename return_t>
      constexpr member_meta<class_t, return_t> member_ptr_meta(return_t class_t::*){}
      template<typename class_t, typename return_t, typename... args_t>
      constexpr member_meta<class_t, return_t, args_t...> member_ptr_meta(return_t (class_t::*)(args_t...)){}

      template<typename M>
      using member_ptr_meta_t = decltype(member_ptr_meta(std::declval<M>()));

      template<typename... arg_ts>
      struct is_adapter: std::false_type {};

      template<typename... arg_ts>
      struct is_adapter<adapter<arg_ts...>>: std::true_type {};

      template<template<typename, typename> typename op_t, typename... unique_ts>
      struct unique_types
      {
        using type = decltype(std::tuple(std::declval<unique_ts...>()));
      };

      template<template<typename, typename> typename op_t, typename... unique_ts, typename head_t, typename... tail_ts>
      struct unique_types<op_t, std::tuple<unique_ts...>, head_t, tail_ts...>
      {
        using type = typename std::conditional_t<(op_t<head_t, tail_ts>::value || ...),
            unique_types<op_t, std::tuple<unique_ts...>, tail_ts...>,
            unique_types<op_t, std::tuple<unique_ts..., head_t>, tail_ts...>
          >::type;
      };
    }

    template<typename member_ptr_t>
    using member_class_t = typename details::member_ptr_meta_t<member_ptr_t>::class_t;

    template<typename member_ptr_t>
    using member_value_t = typename details::member_ptr_meta_t<member_ptr_t>::value_t;

    template<typename T>
    constexpr bool is_adapter_v = details::is_adapter<std::remove_cvref_t<T>>::value;

    template<direction dir, typename arg1_t, typename arg2_t>
    using lhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg1_t, arg2_t>;

    template<direction dir, typename arg1_t, typename arg2_t>
    using rhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg2_t, arg1_t>;

    template<typename... arg_ts>
    using unique_types_t = typename details::unique_types<std::is_same, std::tuple<>, arg_ts...>::type;

    template<typename... arg_ts>
    using unique_derived_types_t = typename details::unique_types<std::is_base_of, std::tuple<>, arg_ts...>::type;

    template<typename converter_t, typename arg_t>
    using converted_t = std::invoke_result_t<converter_t, arg_t>;
  }

  namespace concepts
  {
    template<typename T>
    concept member_ptr = std::is_member_pointer_v<T>;

    template<typename value_t, typename index_t = std::size_t>
    concept indexable = requires(value_t t, index_t index)
    {
      t[index];
    };

    template<typename T>
    concept dereferencable = requires(T t)
    {
      *t;
      requires (!std::same_as<void, decltype(*t)>);
    };

    template<typename T>
    concept reference = std::is_reference_v<T>;

    template<typename adaptee_t, typename reader_t>
    concept readable = requires(reader_t reader, adaptee_t&& adaptee)
    {
      { reader(FWD(adaptee)) };
      requires (!std::same_as<void, decltype(reader(FWD(adaptee)))>);
    };

    template<typename T>
    concept adapter = traits::is_adapter_v<T>;

    template<typename arg_t, typename adapter_t>
    concept adaptable = requires(adapter_t adapter, arg_t adaptee)
    {
      { adapter(adaptee) };
    };

    template<typename adapter_t>
    concept adaptee_type_known = (adapter<adapter_t> && !std::same_as<typename adapter_t::adaptee_t, details::any>);

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable = requires(mapping_t map)
    {
      requires (dir == direction::rhs_to_lhs
                && requires { { map(std::declval<rhs_t>()) }; })
                || requires { { map(std::declval<lhs_t>()) }; };
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable_assign = requires(mapping_t map, lhs_t lhs, rhs_t rhs)
    {
      map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, direction dir>
    concept mappable_equal = requires(mapping_t map, lhs_t lhs, rhs_t rhs)
    {
      map.template equal<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
    };

    template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
    concept assignable_from_converted = requires(traits::lhs_t<dir, lhs_t, rhs_t> lhs, traits::converted_t<converter_t, traits::rhs_t<dir, lhs_t, rhs_t>> rhs)
    {
      { lhs = rhs };
    };

    template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
    concept equality_comparable_with_converted = requires(traits::lhs_t<dir, lhs_t, rhs_t> lhs, traits::converted_t<converter_t, traits::rhs_t<dir, lhs_t, rhs_t>> rhs)
    {
      { lhs == rhs } -> std::convertible_to<bool>;
    };

    // Credit: https://en.cppreference.com/w/cpp/ranges/range
    template<typename range_t>
    concept range = requires(std::remove_reference_t<range_t>& r)
    {
      std::begin(r); // equality-preserving for forward iterators
      std::end  (r);
    };

    template<typename cont_t>
    concept fixed_size_container = std::is_array_v<std::remove_reference_t<cont_t>> || (range<cont_t> && requires (cont_t c)
    {
      requires (decltype(std::span{ c })::extent != std::dynamic_extent);
    });

    // Very rudimental concept based on "Member Function Table" here: https://en.cppreference.com/w/cpp/container
    template<typename cont_t>
    concept sequence_container = range<cont_t>
      && requires(cont_t c){ { c.size() }; }
      && (requires(cont_t c){ { c.data() }; } || requires(cont_t c){ { c.resize(0) }; });

    // Very rudimental concept based on "Member Function Table" here: https://en.cppreference.com/w/cpp/container
    template<typename cont_t>
    concept associative_container = range<cont_t> && requires(cont_t container)
    {
      typename std::remove_cvref_t<cont_t>::key_type;
    };

    template<typename obj_t>
    concept trivially_copyable = std::is_trivially_copyable_v<std::remove_reference_t<obj_t>>;

    template<typename cont_t>
    concept mapping_container = associative_container<cont_t> && requires
    {
      typename std::remove_cvref_t<cont_t>::mapped_type;
    };

    template<typename cont_t>
    concept resizable = requires(std::remove_reference_t<cont_t> container)
    {
      container.resize(std::size_t{0});
    };

    template<typename from_t, typename to_t>
    concept castable_to = requires
    {
      static_cast<to_t>(std::declval<from_t>());
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

      template<concepts::mapping_container cont_t>
      auto get_mapped() -> typename std::remove_reference_t<cont_t>::mapped_type;
      template<concepts::associative_container cont_t>
      auto get_mapped() -> typename std::remove_reference_t<cont_t>::value_type;
    }

    template<typename T>
    constexpr bool is_mapping_v = details::is_mapping<std::remove_cvref_t<T>>::value;

    template<typename arg_t, concepts::adapter... adapter_ts>
    constexpr std::size_t adaptable_count_v = (concepts::adaptable<arg_t, adapter_ts> +...);

    template<concepts::adapter adapter_t, typename adaptee_t>
    using adapted_t = converted_t<adapter_t, adaptee_t>;

    template<concepts::range range_t>
    using range_value_t = std::remove_reference_t<decltype(*std::begin(std::declval<range_t&>()))>;

    template<concepts::range range_t>
    using range_value_forwarded_t = std_ext::like_t<range_t, traits::range_value_t<range_t>>;

    template<concepts::fixed_size_container cont_t>
    constexpr auto range_size_v = std::size(std::remove_reference_t<cont_t>{});

    template<concepts::associative_container cont_t>
    using mapped_value_t = std::remove_reference_t<decltype(details::get_mapped<cont_t>())>;

    template<concepts::associative_container cont_t>
    using mapped_value_forwarded_t = std_ext::like_t<cont_t, traits::mapped_value_t<cont_t>>;
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
