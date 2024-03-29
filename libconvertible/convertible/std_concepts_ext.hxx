#pragma once

#include <convertible/std_concepts_compat.hxx>

#include <concepts>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
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
      constexpr auto
      member_ptr_meta(return_t class_t::*) -> member_meta<class_t, return_t>
      {}

      template<typename class_t, typename return_t, typename... args_t>
      constexpr auto
      member_ptr_meta(return_t (class_t::*)(args_t...)) -> member_meta<class_t, return_t, args_t...>
      {}

      template<typename M>
      using member_ptr_meta_t = decltype(member_ptr_meta(std::declval<M>()));

      template<template<typename, typename> typename op_t, typename... unique_ts>
      struct unique_types
      {
        using type = decltype(std::tuple(std::declval<unique_ts...>()));
      };

      template<template<typename, typename> typename op_t, typename... unique_ts, typename head_t,
               typename... tail_ts>
      struct unique_types<op_t, std::tuple<unique_ts...>, head_t, tail_ts...>
      {
        using type = typename std::conditional_t<
          (op_t<head_t, tail_ts>::value || ...),
          unique_types<op_t, std::tuple<unique_ts...>, tail_ts...>,
          unique_types<op_t, std::tuple<unique_ts..., head_t>, tail_ts...>>::type;
      };
    }

    template<typename as_t, typename with_t>
    using like_t = decltype(std::forward_like<as_t>(std::declval<with_t>()));

    template<typename member_ptr_t>
    using member_class_t = typename details::member_ptr_meta_t<member_ptr_t>::class_t;

    template<typename member_ptr_t>
    using member_value_t = typename details::member_ptr_meta_t<member_ptr_t>::value_t;

    template<typename... arg_ts>
    using unique_ts = typename details::unique_types<std::is_same, std::tuple<>, arg_ts...>::type;

    template<typename... arg_ts>
    using unique_derived_ts =
      typename details::unique_types<std::is_base_of, std::tuple<>, arg_ts...>::type;
  }

  namespace concepts
  {
    template<typename obj_t>
    concept trivially_copyable = std::is_trivially_copyable_v<std::remove_reference_t<obj_t>>;

    template<typename from_t, typename to_t>
    concept castable_to = requires { static_cast<to_t>(std::declval<from_t>()); };

    template<typename T>
    concept member_ptr = std::is_member_pointer_v<T>;

    // Credit: https://en.cppreference.com/w/cpp/ranges/range
    template<typename T>
    concept range = requires (T& t) {
                      std::begin(t);
                      std::end(t);
                    };

    template<typename T, typename index_t = std::size_t>
    concept indexable = requires (T& t) { t[std::declval<index_t>()]; };

    template<typename T>
    concept dereferencable = requires (T t) {
                               *t;
                               requires (!std::same_as<void, decltype(*t)>);
                             };

    template<typename cont_t>
    concept fixed_size_container =
      std::is_array_v<std::remove_reference_t<cont_t>> ||
      (range<cont_t> &&
       requires (cont_t c) { requires (decltype(std::span{c})::extent != std::dynamic_extent); });

    template<typename cont_t>
    concept resizable_container =
      range<cont_t> && requires (std::remove_reference_t<cont_t> c) { c.resize(std::size_t{0}); };

    // Very rudimental concept based on "Member Function Table" here:
    // https://en.cppreference.com/w/cpp/container
    template<typename cont_t>
    concept sequence_container = range<cont_t> &&
                                 requires (cont_t c) {
                                   {
                                     std::size(c)
                                   };
                                 } &&
                                 (
                                   requires (cont_t c) {
                                     {
                                       std::data(c)
                                     };
                                   } ||
                                   requires (std::remove_cvref_t<cont_t> c) {
                                     {
                                       c.resize(0)
                                     };
                                   });

    // Very rudimental concept based on "Member Function Table" here:
    // https://en.cppreference.com/w/cpp/container
    template<typename cont_t>
    concept associative_container =
      range<cont_t> &&
      requires (cont_t container) { typename std::remove_cvref_t<cont_t>::key_type; };

    template<typename cont_t>
    concept mapping_container = associative_container<cont_t> &&
                                requires { typename std::remove_cvref_t<cont_t>::mapped_type; };
  }

  namespace traits
  {
    namespace details
    {
      template<concepts::mapping_container cont_t>
      auto get_mapped() -> typename std::remove_reference_t<cont_t>::mapped_type;
      template<concepts::associative_container cont_t>
      auto get_mapped() -> typename std::remove_reference_t<cont_t>::value_type;
    }

    template<concepts::range range_t>
    using range_value_t = std::remove_reference_t<decltype(*std::begin(std::declval<range_t&>()))>;

    template<concepts::range range_t>
    using range_value_forwarded_t = like_t<range_t, range_value_t<range_t>>;

    template<concepts::fixed_size_container cont_t>
    constexpr auto range_size_v = std::size(std::remove_reference_t<cont_t>{});

    template<concepts::associative_container cont_t>
    using mapped_value_t = std::remove_reference_t<decltype(details::get_mapped<cont_t>())>;

    template<concepts::associative_container cont_t>
    using mapped_value_forwarded_t = like_t<cont_t, mapped_value_t<cont_t>>;

    namespace details
    {
      template<typename T>
      struct container_meta
      {};

      template<template<typename, typename...> typename cont_t, typename... _arg_ts>
        requires concepts::sequence_container<cont_t<_arg_ts...>>
      struct container_meta<cont_t<_arg_ts...>>
      {
        template<typename new_elem_t>
        using with_elem_t = decltype(cont_t{std::declval<new_elem_t>()});
      };

      template<template<typename, typename...> typename cont_t, typename... _arg_ts>
        requires concepts::associative_container<cont_t<_arg_ts...>> &&
                 (!concepts::mapping_container<cont_t<_arg_ts...>>)
      struct container_meta<cont_t<_arg_ts...>>
      {
        template<typename new_elem_t>
        using with_elem_t = decltype(cont_t{std::declval<new_elem_t>()});
      };

      template<template<typename, typename...> typename cont_t, typename... _arg_ts>
        requires concepts::mapping_container<cont_t<_arg_ts...>>
      struct container_meta<cont_t<_arg_ts...>>
      {
        using key_t = typename std::remove_reference_t<cont_t<_arg_ts...>>::key_type;

        template<typename new_elem_t>
        using with_elem_t = decltype(cont_t{std::declval<std::pair<key_t, new_elem_t>>()});
      };
    }

    template<typename cont_t, typename new_elem_t>
    using as_container_t =
      traits::like_t<cont_t, typename details::container_meta<
                               std::remove_cvref_t<cont_t>>::template with_elem_t<new_elem_t>>;
  }
}

#undef FWD
