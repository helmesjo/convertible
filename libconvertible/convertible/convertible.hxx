#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

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

namespace std
{
  template<typename rhs_t, typename lhs_t>
  concept assignable_to = std::assignable_from<lhs_t, rhs_t>;
}

namespace convertible
{
  // Workaround for MSVC bug: https://developercommunity2.visualstudio.com/t/Concept-satisfaction-failure-for-member/1489332
#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define DIR_DECL(...) int
#define DIR_READ(...) msvc_unpack_enum<__VA_ARGS__> // For bug with enum var used in requires-clause

  namespace direction
  {
    static constexpr int lhs_to_rhs = 0;
    static constexpr int rhs_to_lhs = 1;
  }

  template<DIR_DECL(direction) dir>
    constexpr auto msvc_unpack_enum = dir;
#else
#define DIR_DECL(...) __VA_ARGS__
#define DIR_READ(...) __VA_ARGS__

  enum class direction
  {
    lhs_to_rhs,
    rhs_to_lhs
  };
#endif

  namespace std_ext
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
    const_value(const elem_t (&str)[size]) -> const_value<elem_t[size]>;

  }

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

    template<DIR_DECL(direction) dir, typename arg1_t, typename arg2_t>
    using lhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg1_t, arg2_t>;

    template<DIR_DECL(direction) dir, typename arg1_t, typename arg2_t>
    using rhs_t = std::conditional_t<dir == direction::rhs_to_lhs, arg2_t, arg1_t>;

    template<typename... arg_ts>
    using unique_types_t = typename details::unique_types<std::is_same, std::tuple<>, arg_ts...>::type;

    template<typename... arg_ts>
    using unique_derived_types_t = typename details::unique_types<std::is_base_of, std::tuple<>, arg_ts...>::type;

    template<typename converter_t, typename arg_t>
    using converted_t = std::invoke_result_t<converter_t, arg_t>;

    template<typename as_t, typename with_t>
    using like_t = decltype(std_ext::forward_like<as_t>(std::declval<with_t>()));
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

    template<typename mapping_t, typename lhs_t, typename rhs_t, DIR_DECL(direction) dir>
    concept mappable = requires(mapping_t map)
    {
      requires (dir == direction::rhs_to_lhs
                && requires { { map(std::declval<rhs_t>()) }; })
                || requires { { map(std::declval<lhs_t>()) }; };
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, DIR_DECL(direction) dir>
    concept mappable_assign = requires(mapping_t map, lhs_t lhs, rhs_t rhs)
    {
      map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
    };

    template<typename mapping_t, typename lhs_t, typename rhs_t, DIR_DECL(direction) dir>
    concept mappable_equal = requires(mapping_t map, lhs_t lhs, rhs_t rhs)
    {
      map.template equal<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
    };

    template<DIR_DECL(direction) dir, typename callable_t, typename arg1_t, typename arg2_t, typename converter_t>
    concept executable_with = requires(callable_t callable, traits::lhs_t<dir, arg1_t, arg2_t> l, traits::rhs_t<dir, arg1_t, arg2_t> r, converter_t converter)
    {
      { callable.template operator()<dir>(l, r, converter) };
    };

    template<DIR_DECL(direction) dir, typename lhs_t, typename rhs_t, typename converter_t>
    concept assignable_from_converted = requires(traits::lhs_t<dir, lhs_t, rhs_t> lhs, traits::converted_t<converter_t, traits::rhs_t<dir, lhs_t, rhs_t>> rhs)
    {
      { lhs = rhs };
    };

    template<DIR_DECL(direction) dir, typename lhs_t, typename rhs_t, typename converter_t>
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
    using range_value_forwarded_t = traits::like_t<range_t, traits::range_value_t<range_t>>;

    template<concepts::fixed_size_container cont_t>
    constexpr auto range_size_v = std::size(std::remove_reference_t<cont_t>{});

    template<concepts::associative_container cont_t>
    using mapped_value_t = std::remove_reference_t<decltype(details::get_mapped<cont_t>())>;

    template<concepts::associative_container cont_t>
    using mapped_value_forwarded_t = traits::like_t<cont_t, traits::mapped_value_t<cont_t>>;
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

  namespace reader
  {
    template<typename adaptee_t = details::any>
    struct identity
    {
      constexpr decltype(auto) operator()(auto&& obj) const
        requires std::is_same_v<adaptee_t, details::any>
      {
        return FWD(obj);
      }

      constexpr decltype(auto) operator()(auto&& obj) const
        requires std::common_reference_with<adaptee_t, decltype(obj)>
      {
        return FWD(obj);
      }
    };

    template<concepts::member_ptr member_ptr_t>
    struct member
    {
      using class_t = traits::member_class_t<member_ptr_t>;

      constexpr member(member_ptr_t ptr):
        ptr_(std::move(ptr))
      {}

      template<typename obj_t>
        requires std::derived_from<std::remove_reference_t<obj_t>, class_t>
      constexpr decltype(auto) operator()(obj_t&& obj) const
      {
        // MSVC bug: 'FWD(obj).*ptr' causes 'fatal error C1001: Internal compiler error'
        //            when 'obj' is r-value reference (in combination with above 'requires' etc.)
        if constexpr(std::is_member_object_pointer_v<member_ptr_t>)
          return FWD(obj).*ptr_;
        if constexpr(std::is_member_function_pointer_v<member_ptr_t>)
          return (FWD(obj).*ptr_)();
      }

      member_ptr_t ptr_;
    };

    template<details::const_value i>
    struct index
    {
      constexpr decltype(auto) operator()(concepts::indexable<decltype(i)> auto&& obj) const
      {
        // standard containers do not have a 'move from' index operator (for legacy reasons)
        // but here we want to support it for performance reasons.
        if constexpr (std::is_rvalue_reference_v<decltype(obj)>)
        {
          return std::move(FWD(obj)[i]);
        }
        else
        {
          return FWD(obj)[i];
        }
      }
    };

    struct deref
    {
      constexpr decltype(auto) operator()(concepts::dereferencable auto&& obj) const
      {
        return *FWD(obj);
      }
    };

    struct maybe
    {

      template<concepts::dereferencable object_t>
      struct maybe_wrapper
      {
        using value_t = std::remove_reference_t<decltype(*std::declval<object_t>())>;
        using object_ref_t = std::remove_reference_t<object_t>&;

        maybe_wrapper(object_ref_t obj)
        : obj_(obj)
        {}

        auto& object_or_defaulted()
        {
          if(obj_)
            return obj_;
          else
            return defaulted_;
        }
        const auto& object_or_defaulted() const
        {
          return const_cast<maybe_wrapper&>(*this).object_or_defaulted();
        }

        decltype(auto) operator*()
        {
          if(obj_)
          {
            return *std::forward<object_t>(obj_);
          }
          else
          {
            defaulted_ = value_t{};
            return *std::forward<object_t>(defaulted_);
          }
        }

        constexpr operator object_t&() { return object_or_defaulted(); }
        constexpr operator const object_t&() const { return object_or_defaulted(); }
        constexpr operator bool() const { return true; }
        decltype(auto) operator<=>(auto&& rhs) const { return obj_ <=> FWD(rhs); }
        decltype(auto) operator==(auto&& rhs) const { return obj_ == FWD(rhs); }
        decltype(auto) operator!=(auto&& rhs) const { return obj_ != FWD(rhs); }
        decltype(auto) operator=(auto&& rhs) { return obj_ = FWD(rhs); }

        object_ref_t obj_;
        value_t defaultedValue_{};
        std::remove_reference_t<object_t> defaulted_{ value_t{} };
      };
      template<concepts::dereferencable obj_t>
      maybe_wrapper(obj_t&& obj) -> maybe_wrapper<decltype(obj)>;

      template<concepts::dereferencable object_t,
                               typename value_t = std::remove_cvref_t<decltype(*std::declval<object_t>())>>
      constexpr decltype(auto) operator()(object_t&& obj) const
        requires std::constructible_from<bool, object_t> &&
                 std::is_assignable_v<object_t, value_t>
      {
        return maybe_wrapper<decltype(obj)>(obj);
      }
    };

    template<std::size_t first, std::size_t last>
      requires (first <= last)
    struct binary
    {
      template<concepts::fixed_size_container cont_t>
      static constexpr std::size_t range_fixed_size_bytes =
        std::size(std::remove_reference_t<cont_t>{}) * sizeof(traits::range_value_t<cont_t>);

      template<concepts::sequence_container cont_t>
      static constexpr std::size_t range_size_bytes(cont_t&& cont)
      {
        return std::size(FWD(cont)) * sizeof(traits::range_value_t<cont_t>);
      }

      template<concepts::sequence_container storage_t, std::size_t _first, std::size_t _last>
        requires (_first <= _last)
              && concepts::trivially_copyable<traits::range_value_t<std::remove_reference_t<storage_t>>>
      struct binary_proxy
      {
        using storage_value_t = traits::range_value_t<std::remove_reference_t<storage_t>>;
        using byte_t = std::remove_reference_t<traits::like_t<storage_value_t, std::byte>>;
        using const_byte_t = std::add_const_t<byte_t>;

        static constexpr bool dynamic = (!concepts::fixed_size_container<storage_t>);
        static constexpr std::size_t first_byte = _first;
        static constexpr std::size_t last_byte = _last;
        static constexpr std::size_t byte_count = last_byte - first_byte + 1;
        static constexpr std::size_t bytes_required = first_byte + byte_count;

        // trivial type
        constexpr binary_proxy(concepts::trivially_copyable auto& obj)
          requires (!concepts::range<decltype(obj)>) && std::is_same_v<std::remove_cvref_t<storage_t>, std::span<byte_t>>
                && (sizeof(obj) >= byte_count)
        : bytes_(std::span{reinterpret_cast<byte_t*>(std::addressof(obj)), byte_count})
        {
        }

        // fixed size container
        constexpr binary_proxy(concepts::fixed_size_container auto&& bytes)
          requires (range_fixed_size_bytes<decltype(bytes)> >= byte_count)
        : bytes_(bytes)
        {
        }

        // dynamic container
        constexpr binary_proxy(concepts::sequence_container auto&& bytes)
          requires (!concepts::fixed_size_container<decltype(bytes)>)
        : bytes_(bytes)
        {
          if constexpr(concepts::resizable<storage_t>)
          {
            if(range_size_bytes(bytes_) < bytes_required)
            {
              const std::size_t newSize = std::max(std::size_t{1}, bytes_required / sizeof(storage_value_t));
              bytes_.resize(newSize);
            }
          }
          if(range_size_bytes(bytes_) < bytes_required)
          {
            throw std::overflow_error(
              "[binary_proxy::ctor] bytes(storage) " + std::to_string(range_size_bytes(bytes_))
              + " < bytes(proxy) " + std::to_string(size())
            );
          }
        }

        // make `trivially_copyable<binary_proxy> == false`
        ~binary_proxy(){}

        constexpr byte_t* data() noexcept
          requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
        {
          return reinterpret_cast<byte_t*>(std::data(bytes_)) + first_byte;
        }

        constexpr const_byte_t* data() const noexcept
        {
          return reinterpret_cast<const_byte_t*>(std::data(bytes_)) + first_byte;
        }

        constexpr std::size_t size() const noexcept
        {
          return byte_count;
        }

        constexpr decltype(auto) storage()
          requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
        {
          return bytes_;
        }

        constexpr decltype(auto) storage() const
          requires std::is_const_v<std::remove_reference_t<storage_t>>
        {
          return bytes_;
        }

        // span<const_bytes_t> (implicit conversion)
        constexpr operator const std::span<const_byte_t>() const noexcept
        {
          return {data(), size()};
        }

        // trivial type (non-const lvalue reference)
        template<concepts::trivially_copyable to_t>
          requires (!concepts::range<to_t>)
                && (sizeof(to_t) >= byte_count)
                && (!std::is_const_v<std::remove_reference_t<storage_t>>)
        constexpr operator to_t&()
        {
          return reinterpret_cast<to_t&>(*data());
        }

        // trivial type (lvalue)
        template<concepts::trivially_copyable to_t>
          requires (!concepts::range<to_t>)
                && (sizeof(to_t) >= byte_count)
        constexpr explicit operator to_t() const
        {
          return reinterpret_cast<const to_t&>(*data());
        }

        // trivial type
        template<concepts::trivially_copyable obj_t>
        constexpr binary_proxy& operator=(const obj_t& obj)
          requires (!concepts::fixed_size_container<obj_t>) &&
                   (sizeof(obj) <= byte_count)
        {
          *this = {reinterpret_cast<const std::byte*>(std::addressof(obj)), sizeof(obj)};
          return *this;
        }

        // fixed size container
        template<concepts::fixed_size_container cont_t>
        constexpr binary_proxy& operator=(const cont_t& range)
          requires concepts::trivially_copyable<traits::range_value_t<cont_t>> &&
                   (range_fixed_size_bytes<cont_t> <= byte_count)
        {
          *this = {reinterpret_cast<const std::byte*>(std::data(range)), std::size(range)};
          return *this;
        }

        // trivial type
        template<concepts::trivially_copyable obj_t>
        constexpr bool operator==(const obj_t& obj) const
          requires (!concepts::fixed_size_container<obj_t>) &&
                   (sizeof(obj) <= byte_count)
        {
          return *this == std::span{reinterpret_cast<const std::byte*>(std::addressof(obj)), sizeof(obj)};
        }

        // fixed size container
        template<concepts::fixed_size_container cont_t>
        constexpr bool operator==(const cont_t& range) const
          requires concepts::trivially_copyable<traits::range_value_t<cont_t>> &&
                   (range_fixed_size_bytes<cont_t> <= byte_count)
        {
          return *this == std::span{reinterpret_cast<const std::byte*>(std::data(range)), std::size(range)};
        }

      private:
        constexpr binary_proxy& operator=(std::span<const std::byte> src)
        {
          if constexpr(concepts::resizable<storage_t>)
          {
            if(src.size() > range_size_bytes(bytes_))
            {
              throw std::overflow_error(
                "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_))
                + " > bytes(storage) " + std::to_string(range_size_bytes(bytes_))
                + " - storage resized after binary_proxy construction"
              );
            }
          }
          std::memcpy(data(), std::data(src), std::size(src));
          return *this;
        }

        constexpr bool operator==(std::span<const std::byte> src) const
        {
          if constexpr(concepts::resizable<storage_t>)
          {
            if(src.size() > range_size_bytes(bytes_))
            {
              throw std::overflow_error(
                "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_))
                + " > bytes(storage) " + std::to_string(range_size_bytes(bytes_))
                + " - storage resized after binary_proxy construction"
              );
            }
          }
          return std::memcmp(data(), std::data(src), std::size(src)) == 0;
        }

        storage_t bytes_;
      };

      // trivial type (stored as std::span)
      template<concepts::trivially_copyable obj_t, std::size_t _first = first, std::size_t _last = last>
        requires (!concepts::range<obj_t>)
      binary_proxy(obj_t&) ->
        binary_proxy<std::span<std::remove_reference_t<traits::like_t<obj_t, std::byte>>>, _first, _last>;

      // sequence container (dynamic or fixed size)
      template<concepts::sequence_container bytes_t, std::size_t _first = first, std::size_t _last = last>
      binary_proxy(bytes_t&) -> binary_proxy<std::remove_reference_t<bytes_t>&, _first, _last>;

      constexpr decltype(auto) operator()(auto&& obj) const
        requires requires{ binary_proxy(FWD(obj)); }
      {
        return binary_proxy(FWD(obj));
      }

      // binary_proxy (by value to avoid all the different kinds of overloads)
      // creates a new proxy starting at one-past-last using the same storage
      template<typename _bytes_t, std::size_t _first, std::size_t _last, template<typename, std::size_t, std::size_t> typename _binary_proxy>
      constexpr decltype(auto) operator()(_binary_proxy<_bytes_t, _first, _last> byteProxy) const
        requires std::same_as<typename binary<_first, _last>::template binary_proxy<_bytes_t, _first, _last>,
                                                                      _binary_proxy<_bytes_t, _first, _last>>
      {
        return binary_proxy<_bytes_t, _last+1 + first, _last+1 + last>(byteProxy.storage());
      }
    };

    template<concepts::adapter... adapter_ts>
    struct composed
    {
      using adapters_t = std::tuple<adapter_ts...>;

      constexpr composed(adapter_ts... adapters):
        adapters_(std::move(adapters)...)
      {}

      static constexpr decltype(auto) compose(auto&& arg, concepts::adapter auto&& adapter)
        requires concepts::adaptable<decltype(arg), decltype(adapter)>
      {
        return FWD(adapter)(FWD(arg));
      }

      static constexpr decltype(auto) compose(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
        requires requires(){ FWD(head)(compose(FWD(arg), FWD(tail)...)); }
      {
        return FWD(head)(compose(FWD(arg), FWD(tail)...));
      }

      template<typename arg_t>
      constexpr decltype(auto) operator()(arg_t&& arg) const
        requires requires(){ compose(FWD(arg), std::declval<adapter_ts>()...); }
      {
        return std::apply([&arg](auto&&... adapters) -> decltype(auto) {
          return compose(std::forward<arg_t>(arg), FWD(adapters)...);
        }, adapters_);
      }

      adapters_t adapters_;
    };
  }

  template<typename _adaptee_t = details::any, typename reader_t = reader::identity<_adaptee_t>>
  struct adapter
  {
    using adaptee_t = _adaptee_t;
    using adaptee_value_t = std::remove_reference_t<adaptee_t>;
    static constexpr bool accepts_any_adaptee = std::is_same_v<adaptee_value_t, details::any>;

    constexpr adapter() = default;
    constexpr adapter(const adapter&) = default;
    constexpr adapter(adapter&&) = default;
    constexpr explicit adapter(adaptee_t adaptee, reader_t reader)
      : reader_(FWD(reader)),
        adaptee_(adaptee)
    {
    }
    constexpr explicit adapter(reader_t reader)
      : reader_(FWD(reader))
    {
    }

    constexpr decltype(auto) operator()(concepts::readable<reader_t> auto&& obj) const
    {
      return reader_(FWD(obj));
    }

    // allow implicit/explicit conversion to adaptee_t (preserving type qualifiers of `obj`)
    template<typename obj_t>
    constexpr decltype(auto) operator()(obj_t&& obj) const
      requires (!concepts::readable<decltype(obj), reader_t>)
            && concepts::castable_to<obj_t, traits::like_t<decltype(obj), adaptee_t>>
            && concepts::readable<traits::like_t<decltype(obj), adaptee_t>, reader_t>
    {
      return reader_(static_cast<traits::like_t<decltype(obj), adaptee_t>>(FWD(obj)));
    }

    constexpr auto defaulted_adaptee() const
      requires (!accepts_any_adaptee)
    {
      return adaptee_;
    }

    reader_t reader_;
    const adaptee_value_t adaptee_{};
  };

  template<concepts::adapter... adapter_ts>
  constexpr auto compose(adapter_ts&&... adapters)
  {
    auto adaptee = std::get<sizeof...(adapters)-1>(std::tuple{adapters...}).defaulted_adaptee();
    return adapter(adaptee, reader::composed(FWD(adapters)...));
  }

  constexpr auto custom(auto&& reader, concepts::readable<decltype(reader)> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., FWD(reader));
  }

  constexpr auto custom(auto&& reader, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(custom(FWD(reader)), FWD(inner)...);
  }

  constexpr auto identity(concepts::readable<reader::identity<>> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::identity<std::remove_reference_t<decltype(adaptee)>...>{});
  }

  constexpr auto identity(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(identity(), FWD(inner)...);
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t ptr, concepts::readable<reader::member<member_ptr_t>> auto&&... adaptee)
  {
    return adapter<traits::member_class_t<member_ptr_t>, reader::member<member_ptr_t>>(FWD(adaptee)..., FWD(ptr));
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t&& ptr, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(member(FWD(ptr)), FWD(inner)...);
  }

  template<details::const_value i>
  constexpr auto index(concepts::readable<reader::index<i>> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::index<i>{});
  }

  template<details::const_value i>
  constexpr auto index(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(index<i>(), FWD(inner)...);
  }

  constexpr auto deref(concepts::readable<reader::deref> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::deref{});
  }

  constexpr auto deref(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(deref(), FWD(inner)...);
  }

  constexpr auto maybe(concepts::readable<reader::maybe> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::maybe{});
  }

  constexpr auto maybe(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(maybe(), FWD(inner)...);
  }

  template<std::size_t first, std::size_t last>
    requires (first <= last)
  constexpr auto binary(concepts::readable<reader::binary<first, last>> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::binary<first, last>{});
  }

  template<std::size_t first, std::size_t last>
    requires (first <= last)
  constexpr auto binary(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(binary<first, last>(), FWD(inner)...);
  }

  namespace converter
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
              || concepts::castable_to<converted_t<decltype(obj)>, to_t>
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

  namespace operators
  {
    template<typename to_t, typename converter_t>
    using explicit_cast = converter::explicit_cast<std::remove_reference_t<to_t>, converter_t>;

    template<concepts::associative_container container_t, std::common_reference_with<traits::mapped_value_t<container_t>> mapped_forward_t>
    struct associative_inserter
    {
      using key_t = typename container_t::key_type;
      using value_t = typename container_t::value_type;
      using reference_t = typename container_t::reference;
      using const_reference_t = typename container_t::const_reference;
      using mapped_value_t = traits::mapped_value_t<container_t>;

      container_t& cont_;
      std::insert_iterator<container_t> inserter_;
      std::conditional_t<concepts::mapping_container<container_t>, key_t, value_t> key_;

      template<std::common_reference_with<key_t> _key_t, typename _mapped_t>
      associative_inserter(concepts::associative_container auto&& cont, const std::pair<_key_t, _mapped_t>& pair)
      : cont_(cont),
        inserter_(cont, std::begin(cont)),
        key_(pair.first)
      {}

      associative_inserter(concepts::associative_container auto&& cont, const key_t& key)
      : cont_(cont),
        inserter_(cont, std::begin(cont)),
        key_(key)
      {}

      operator mapped_forward_t()&
        requires concepts::mapping_container<container_t>
      {
        return std::forward<mapped_forward_t>(cont_[key_]);
      }

      operator mapped_forward_t()&&
        requires concepts::mapping_container<container_t>
      {
        return std::forward<mapped_forward_t>(cont_[key_]);
      }

      operator mapped_value_t&()&
        requires concepts::mapping_container<container_t>
              && (!std::is_same_v<mapped_value_t&, mapped_forward_t>)
      {
        return cont_[key_];
      }

      operator mapped_forward_t()&
        requires (!concepts::mapping_container<container_t>)
      {
        return std::forward<mapped_forward_t>(cont_.at(key_));
      }

      auto& operator=(auto&& value)
        requires requires
                 {
                   this->inserter_ = { this->key_, FWD(value) };
                 }
      {
        inserter_ = { key_, FWD(value) };
        return *this;
      }

      auto& operator=(auto&& value)
        requires requires
                 {
                   this->inserter_ = FWD(value);
                 }
      {
        inserter_ = FWD(value);
        return *this;
      }
    };
    template<concepts::associative_container cont_t>
    associative_inserter(cont_t&& cont, auto&&)
      -> associative_inserter<std::remove_reference_t<decltype(cont)>, traits::like_t<decltype(cont), traits::mapped_value_t<cont_t>>>;

    template<DIR_DECL(direction) dir>
    inline constexpr decltype(auto) execute_ordered_lhs_rhs(auto&& callback, auto&& lhs, auto&& rhs)
    {
      if constexpr(dir == direction::rhs_to_lhs)
        return FWD(callback)(FWD(lhs), FWD(rhs));
      else
        return FWD(callback)(FWD(rhs), FWD(lhs));
    }

    template<DIR_DECL(direction) dir>
    inline constexpr decltype(auto) ordered_lhs_rhs(auto&& lhs, auto&& rhs)
    {
      if constexpr(dir == direction::rhs_to_lhs)
        return std::forward_as_tuple(FWD(lhs), FWD(rhs));
      else
        return std::forward_as_tuple(FWD(rhs), FWD(lhs));
    }

    struct assign
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
        typename converter_t = converter::identity,
        typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), cast_t>
              || requires{ converter.template assign<dir>(FWD(lhs), FWD(rhs)); })
      {
        if constexpr(concepts::mapping<converter_t>)
        {
          converter.template assign<dir>(FWD(lhs), FWD(rhs));
        }
        else
        {
          return execute_ordered_lhs_rhs<dir>([&converter](auto&& lhs, auto&& rhs) -> decltype(auto){
            return FWD(lhs) = cast_t(converter)(FWD(rhs));
          }, FWD(lhs), FWD(rhs));
        }
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
        typename converter_t = converter::identity>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>)
              && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
                 }
      {
        // 1. figure out 'from' & 'to'
        // 2. if 'to' is resizeable: to.resize(from.size())
        // 3. iterate values
        // 4. call self with lhs & rhs respective range values

        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));

        if constexpr(concepts::resizable<decltype(to)>)
        {
          to.resize(from.size());
        }

        const auto size = std::min(lhs.size(), rhs.size());
        auto end = std::begin(rhs);
        std::advance(end, size);
        std::for_each(std::begin(rhs), end,
          [this, lhsItr = std::begin(lhs), &converter](auto&& rhsElem) mutable {
            this->template operator()<dir>(
              std::forward<traits::range_value_forwarded_t<decltype(lhs)>>(*lhsItr++),
              std::forward<traits::range_value_forwarded_t<decltype(rhs)>>(FWD(rhsElem)),
              converter
            );
          }
        );

        return FWD(to);
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, concepts::associative_container lhs_t, concepts::associative_container rhs_t,
        typename converter_t = converter::identity>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>)
              && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
                 }
      {
        // 1. figure out 'from', and use that as 'key range'
        // 2. clear 'to'
        // 3. iterate keys
        // 4. create associative_inserter for lhs & rhs using the key
        // 5. call self with lhs & rhs mapped value respectively (indirectly using inserter)

        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
        to.clear();
        std::for_each(std::begin(from), std::end(from),
          [this, &lhs, &rhs, &converter](auto&& key) mutable {
            auto lhsInserter = associative_inserter(FWD(lhs), key);
            auto rhsInserter = associative_inserter(FWD(rhs), key);
            using lhs_inserter_forward_t = traits::like_t<decltype(lhs), decltype(lhsInserter)>;
            using rhs_inserter_forward_t = traits::like_t<decltype(rhs), decltype(rhsInserter)>;
            using to_mapped_t = traits::mapped_value_t<traits::lhs_t<dir, decltype(lhs), decltype(rhs)>>;
            using cast_t = explicit_cast<to_mapped_t, converter_t>;
            this->template operator()<dir, lhs_inserter_forward_t, rhs_inserter_forward_t, converter_t, cast_t>(
              std::forward<lhs_inserter_forward_t>(lhsInserter),
              std::forward<rhs_inserter_forward_t>(rhsInserter),
              converter
            );
          }
        );

        return FWD(to);
      }
    };

    struct equal
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
        typename converter_t = converter::identity,
        typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {}) const
        requires (concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), cast_t>
              || requires{ converter.template equal<dir>(FWD(lhs), FWD(rhs)); })
      {
        if constexpr(concepts::mapping<converter_t>)
        {
          return converter.template equal<dir>(FWD(lhs), FWD(rhs));
        }
        else
        {
          return execute_ordered_lhs_rhs<dir>([&converter](auto&& lhs, auto&& rhs){
            return lhs == cast_t(converter)(FWD(rhs));
          }, FWD(lhs), FWD(rhs));
        }
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
        typename converter_t = converter::identity,
        typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
        requires (!concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<lhs_t, converter_t>>)
              && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
                 }
      {
        if constexpr(dir == direction::rhs_to_lhs && concepts::resizable<lhs_t>)
        {
          if(lhs.size() != rhs.size())
            return false;
        }
        if constexpr(dir == direction::lhs_to_rhs && concepts::resizable<rhs_t>)
        {
          if(lhs.size() != rhs.size())
            return false;
        }

        return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
          [this, &converter](const auto& lhs, const auto& rhs){
            return this->template operator()<dir>(lhs, rhs, converter);
          });
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<DIR_DECL(direction) dir = direction::rhs_to_lhs, concepts::associative_container lhs_t, concepts::associative_container rhs_t,
        typename converter_t = converter::identity,
        typename cast_t = explicit_cast<traits::mapped_value_t<lhs_t>, converter_t>>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
        requires (!concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<lhs_t, converter_t>>)
              && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
                 }
      {
        return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
          [this, &converter](const auto& lhs, const auto& rhs){
            // key-value pair (map etc.)
            if constexpr(concepts::mapping_container<rhs_t>)
              return lhs.first == rhs.first && this->template operator()<dir>(lhs.second, rhs.second, converter);
            // value (set etc.)
            else
              return this->template operator()<dir>(lhs, rhs, converter);
          });
      }
    };
  }

  template<concepts::adapter _lhs_adapter_t, concepts::adapter _rhs_adapter_t, typename _converter_t = converter::identity>
  struct mapping
  {
    using lhs_adapter_t = _lhs_adapter_t;
    using rhs_adapter_t = _rhs_adapter_t;
    using converter_t = _converter_t;

    constexpr explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter = {}):
      lhsAdapter_(std::move(lhsAdapter)),
      rhsAdapter_(std::move(rhsAdapter)),
      converter_(std::move(converter))
    {}

    template<DIR_DECL(direction) dir>
    constexpr void assign(concepts::adaptable<lhs_adapter_t> auto&& lhs, concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter)
      {
        operators::assign{}.template operator()<dir>(lhsAdapter(FWD(lhs)), rhsAdapter(FWD(rhs)), converter);
      }
    {
      operators::assign{}.template operator()<dir>(lhsAdapter_(FWD(lhs)), rhsAdapter_(FWD(rhs)), converter_);
    }

    template<DIR_DECL(direction) dir = direction::rhs_to_lhs>
    constexpr bool equal(concepts::adaptable<lhs_adapter_t> auto&& lhs, concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter)
      {
        operators::equal{}.template operator()<dir>(lhsAdapter(FWD(lhs)), rhsAdapter(FWD(rhs)), converter);
      }
    {
      return operators::equal{}.template operator()<dir>(lhsAdapter_(FWD(lhs)), rhsAdapter_(FWD(rhs)), converter_);
    }

    template<concepts::adaptable<rhs_adapter_t> rhs_t = typename rhs_adapter_t::adaptee_value_t>
    constexpr auto operator()(concepts::adaptable<lhs_adapter_t> auto&& lhs) const
      requires requires(rhs_t& rhs){ this->assign<direction::lhs_to_rhs>(FWD(lhs), rhs); }
    {
      rhs_t rhs = defaulted_rhs();
      assign<direction::lhs_to_rhs>(FWD(lhs), rhs);
      return rhs;
    }

    template<concepts::adaptable<lhs_adapter_t> lhs_t = typename lhs_adapter_t::adaptee_value_t>
    constexpr auto operator()(concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires(lhs_t& lhs){ this->assign<direction::rhs_to_lhs>(lhs, FWD(rhs)); }
    {
      lhs_t lhs = defaulted_lhs();
      assign<direction::rhs_to_lhs>(lhs, FWD(rhs));
      return lhs;
    }

    constexpr auto defaulted_lhs() const
    {
      return lhsAdapter_.defaulted_adaptee();
    }

    constexpr auto defaulted_rhs() const
    {
      return rhsAdapter_.defaulted_adaptee();
    }

    lhs_adapter_t lhsAdapter_;
    rhs_adapter_t rhsAdapter_;
    converter_t converter_;
  };

  template<typename callback_t, typename tuple_t>
  constexpr bool for_each(callback_t&& callback, tuple_t&& pack)
  {
    return std::apply([&](auto&&... args){
      return (FWD(callback)(FWD(args)) && ...);
    }, FWD(pack));
  }

  template<concepts::mapping... mapping_ts>
  struct mapping_table
  {
    using lhs_unique_types = traits::unique_derived_types_t<typename mapping_ts::lhs_adapter_t::adaptee_value_t...>;
    using rhs_unique_types = traits::unique_derived_types_t<typename mapping_ts::rhs_adapter_t::adaptee_value_t...>;

    constexpr lhs_unique_types defaulted_lhs() const
    {
      lhs_unique_types rets;
      for_each([this](auto&& lhs){
        using lhs_t = std::remove_reference_t<decltype(lhs)>;
        for_each([&lhs](auto&& mapping){
          using defaulted_lhs_t = decltype(mapping.defaulted_lhs());
          if constexpr(std::is_same_v<defaulted_lhs_t, lhs_t>)
          {
            lhs = mapping.defaulted_lhs();
          }
          return true;
        }, mappings_);
        return true;
      }, rets);
      return rets;
    }

    constexpr rhs_unique_types defaulted_rhs() const
    {
      rhs_unique_types rets;
      for_each([this](auto&& rhs){
        using rhs_t = std::remove_reference_t<decltype(rhs)>;
        for_each([&rhs](auto&& mapping){
          using defaulted_rhs_t = decltype(mapping.defaulted_rhs());
          if constexpr(std::is_same_v<defaulted_rhs_t, rhs_t>)
          {
            rhs = mapping.defaulted_rhs();
          }
          return true;
        }, mappings_);
        return true;
      }, rets);
      return rets;
    }

    constexpr explicit mapping_table(mapping_ts... mappings):
      mappings_(std::move(mappings)...)
    {
    }

    template<DIR_DECL(direction) dir, typename lhs_t, typename rhs_t>
    constexpr void assign(lhs_t&& lhs, rhs_t&& rhs) const
      requires (concepts::mappable_assign<mapping_ts, lhs_t, rhs_t, dir> || ...)
    {
      for_each([&lhs, &rhs](concepts::mapping auto&& map){
        if constexpr(concepts::mappable_assign<decltype(map), lhs_t, rhs_t, dir>)
        {
          map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        }
        return true;
      }, mappings_);
    }

    template<DIR_DECL(direction) dir = direction::rhs_to_lhs>
    constexpr bool equal(const auto& lhs, const auto& rhs) const
      requires (concepts::mappable_equal<mapping_ts, decltype(lhs), decltype(rhs), direction::rhs_to_lhs> || ...)
    {
      return for_each([&lhs, &rhs](concepts::mapping auto&& map) -> bool{
        if constexpr(concepts::mappable_equal<decltype(map), decltype(lhs), decltype(rhs), direction::rhs_to_lhs>)
        {
          return map.equal(lhs, rhs);
        }
        return true;
      }, mappings_);
    }

    template<typename lhs_t, typename result_t = rhs_unique_types>
      requires (concepts::adaptee_type_known<typename mapping_ts::rhs_adapter_t> || ...) &&
        (
         traits::adaptable_count_v<lhs_t, typename mapping_ts::lhs_adapter_t...>
         >
         traits::adaptable_count_v<lhs_t, typename mapping_ts::rhs_adapter_t...>
        )
    constexpr auto operator()(lhs_t&& lhs) const
    {
      auto rets = defaulted_rhs();

      for_each([&](auto&& rhs) -> bool{
        if constexpr(requires{ { assign<direction::lhs_to_rhs>(std::forward<lhs_t>(lhs), rhs) }; })
        {
          assign<direction::lhs_to_rhs>(std::forward<lhs_t>(lhs), rhs);
        }
        return true;
      }, FWD(rets));

      if constexpr(std::tuple_size_v<result_t> == 1)
      {
        return std::get<0>(rets);
      }
      else
      {
        return rets;
      }
    }

    template<typename rhs_t, typename result_t = lhs_unique_types>
      requires (concepts::adaptee_type_known<typename mapping_ts::lhs_adapter_t> || ...) &&
        (
         traits::adaptable_count_v<rhs_t, typename mapping_ts::lhs_adapter_t...>
         <
         traits::adaptable_count_v<rhs_t, typename mapping_ts::rhs_adapter_t...>
        )
    constexpr auto operator()(rhs_t&& rhs) const
    {
      auto rets = defaulted_lhs();

      for_each([&](auto&& lhs) -> bool{
        if constexpr(requires{ { assign<direction::rhs_to_lhs>(lhs, std::forward<rhs_t>(rhs)) }; })
        {
          assign<direction::rhs_to_lhs>(lhs, std::forward<rhs_t>(rhs));
        }
        return true;
      }, FWD(rets));

      if constexpr(std::tuple_size_v<result_t> == 1)
      {
        return std::get<0>(rets);
      }
      else
      {
        return rets;
      }
    }

    std::tuple<mapping_ts...> mappings_;
  };

  template<typename... mapping_ts>
  constexpr auto extend(mapping_table<mapping_ts...> table, auto&&... mappings)
  {
    return std::apply([&](auto&&... mappings1){
      return mapping_table(std::forward<mapping_ts>(mappings1)..., std::forward<decltype(mappings)>(mappings)...);
    }, table.mappings_);
  }
}

#undef FWD
#undef DIR_DECL
#undef DIR_READ
