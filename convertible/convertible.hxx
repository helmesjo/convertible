#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
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
#if (__clang_major__ <= 13 && (defined(__APPLE__) || defined(__EMSCRIPTEN__))) || __clang_major__ < 13
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

    template<typename T>
    constexpr decltype(auto) fn_compose (auto&& arg, T&& t)
    {
      return FWD(t)(FWD(arg));
    }

    template<typename F, typename... Rest>
    constexpr decltype(auto) fn_compose(auto&& arg, F&& f, Rest&&... rest)
    {
      return FWD(f)(fn_compose(FWD(arg), FWD(rest)...));
    }
  }

  template<typename obj_t, typename reader_t>
  struct object;

  namespace traits
  {
    namespace details
    {
      template<typename C, typename R>
      struct member_ptr_meta
      {
        using class_t = C;
        using value_t = R;
        constexpr member_ptr_meta(R C::*){}
      };

      template<typename M>
      requires std::is_member_pointer_v<M>
      using member_ptr_meta_t = decltype(member_ptr_meta(std::declval<M>()));

      template<typename... arg_ts>
      struct is_adapter: std::false_type {};

      template<typename... arg_ts>
      struct is_adapter<object<arg_ts...>>: std::true_type {};

      template<DIR_DECL(direction) dir, typename callable_t, typename lhs_t, typename rhs_t, typename converter_t>
      struct executable : std::false_type {};

      template<typename callable_t, typename lhs_t, typename rhs_t, typename converter_t>
      struct executable<direction::rhs_to_lhs, callable_t, lhs_t, rhs_t, converter_t> : std::integral_constant<bool, std::is_invocable_v<callable_t, lhs_t, rhs_t, converter_t>> {};

      template<typename callable_t, typename lhs_t, typename rhs_t, typename converter_t>
      struct executable<direction::lhs_to_rhs, callable_t, lhs_t, rhs_t, converter_t> : std::integral_constant<bool, std::is_invocable_v<callable_t, rhs_t, lhs_t, converter_t>> {};

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
    using range_value_t = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;

    template<typename T>
    constexpr bool is_adapter_v = details::is_adapter<std::remove_cvref_t<T>>::value;

    template<DIR_DECL(direction) dir, typename callable_t, typename lhs_t, typename rhs_t, typename converter_t>
    constexpr bool executable_v = details::executable<dir, callable_t, lhs_t, rhs_t, converter_t>::value;

    template<typename... arg_ts>
    using unique_types_t = typename details::unique_types<std::is_same, std::tuple<>, arg_ts...>::type;

    template<typename... arg_ts>
    using unique_derived_types_t = typename details::unique_types<std::is_base_of, std::tuple<>, arg_ts...>::type;
  }

  namespace concepts
  {
    template<typename T>
    concept member_ptr = std::is_member_pointer_v<std::decay_t<T>>;

    template<typename T>
    concept indexable = requires(T t)
    {
      t[0];
    };

    template<typename T>
    concept dereferencable = requires(T t)
    {
      *t;
    };

    template<typename T>
    concept reference = std::is_reference_v<T>;

    template<typename T>
    concept adapter = traits::is_adapter_v<T>;

    template<typename arg_t, typename adapter_t>
    concept adaptable = requires(adapter_t a, arg_t b)
    {
      { a(b) } -> reference;
    };

    template<typename adapter_t>
    concept adapted_type_known = (adapter<adapter_t> && !std::same_as<typename adapter_t::object_t, details::any>);

    template<typename mapping_t, typename operator_t, DIR_DECL(direction) dir, typename lhs_t, typename rhs_t>
    concept mappable = requires(mapping_t m, lhs_t l, rhs_t r)
    {
      { m.template exec<operator_t, dir>(l, r) };
    };

    template<DIR_DECL(direction) dir, typename callable_t, typename lhs_t, typename rhs_t, typename converter_t>
    concept executable_with = traits::executable_v<dir, callable_t, lhs_t, rhs_t, converter_t>;

    template<typename lhs_t, typename rhs_t, typename converter_t>
    concept assignable_from_converted = std::assignable_from<lhs_t, std::invoke_result_t<converter_t, rhs_t>>;

    template<typename lhs_t, typename rhs_t, typename converter_t>
    concept equality_comparable_with_converted = std::equality_comparable_with<lhs_t, std::invoke_result_t<converter_t, rhs_t>>;

    template<class T>
    concept resizable = requires(T container)
    {
      container.resize(std::size_t{0});
    };

    template<typename from_t, typename to_t>
    concept castable_to = requires {
      static_cast<to_t>(std::declval<from_t>());
    };

    // Credit: https://en.cppreference.com/w/cpp/ranges/range
    template<class T>
    concept range = requires( T& t ) {
      std::begin(t); // equality-preserving for forward iterators
      std::end  (t);
    };
  }

  namespace traits
  {
    template<typename arg_t, concepts::adapter... adapter_ts>
    constexpr std::size_t adaptable_count_v = (concepts::adaptable<arg_t, adapter_ts> +...);
  }

  namespace reader
  {
    struct identity
    {
      constexpr decltype(auto) operator()(auto&& obj) const
      {
        return FWD(obj);
      }
    };

    template<typename... adapter_ts>
    struct composed
    {
      constexpr composed(adapter_ts... adapters):
        adapters_(std::move(adapters)...)
      {}

      constexpr decltype(auto) operator()(auto&& obj) const
        requires requires(adapter_ts... args){ details::fn_compose(FWD(obj), args...); }
      {
        return std::apply([&obj](auto&&... args) -> decltype(auto) {
          return details::fn_compose(FWD(obj), args...);
        }, adapters_);
      }

      std::tuple<adapter_ts...> adapters_;
    };

    template<concepts::member_ptr member_ptr_t>
    struct member
    {
      using class_t = traits::member_class_t<member_ptr_t>;

      constexpr member(member_ptr_t ptr):
        ptr_(std::move(ptr))
      {}

      template<typename obj_t>
        requires std::derived_from<class_t, std::decay_t<obj_t>>
      constexpr decltype(auto) operator()(obj_t&& obj) const
      {
        return obj.*ptr_;
      }

      member_ptr_t ptr_;
    };

    template<std::size_t i>
    struct index
    {
      constexpr decltype(auto) operator()(concepts::indexable auto&& obj) const
      {
        return obj[i];
      }
    };

    struct deref
    {
      constexpr decltype(auto) operator()(concepts::dereferencable auto&& obj) const
      {
        return *obj;
      }
    };
  }

  template<typename reader_t = reader::identity, typename adapted_t = details::any>
  struct object
  {
    using object_t = adapted_t;
    using object_decay_t = std::remove_pointer_t<std::decay_t<object_t>>;

    constexpr object() = default;
    constexpr object(const object&) = default;
    constexpr object(object&&) = default;
    constexpr explicit object(reader_t reader)
      : reader_(FWD(reader))
    {
    }

    constexpr decltype(auto) operator()(auto&& obj) const
      requires std::invocable<reader_t, decltype(obj)>
    {
      if constexpr (std::is_rvalue_reference_v<decltype(obj)>)
      {
        return std::move(reader_(FWD(obj)));
      }
      else
      {
        return reader_(FWD(obj));
      }
    }

    reader_t reader_;
  };

  template<concepts::adapter... adapter_ts>
  consteval auto compose(adapter_ts&&... adapters)
  {
    return object(reader::composed(FWD(adapters)...));
  }

  template<concepts::member_ptr member_ptr_t>
  consteval auto member(member_ptr_t ptr)
  {
    return object<reader::member<member_ptr_t>, traits::member_class_t<member_ptr_t>*>(ptr);
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t ptr, concepts::adapter auto&& inner)
  {
    auto read = reader::composed(reader::member<member_ptr_t>{ptr}, inner);
    return object<decltype(read), traits::member_class_t<member_ptr_t>*>(read);
  }

  template<std::size_t i>
  constexpr auto index(concepts::adapter auto&& inner)
  {
    return object(reader::composed(reader::index<i>{}, FWD(inner)));
  }

  template<std::size_t i>
  consteval auto index()
  {
    return object(reader::index<i>{});
  }

  constexpr auto deref(concepts::adapter auto&& inner)
  {
    return object(reader::composed(reader::deref{}, FWD(inner)));
  }

  consteval auto deref()
  {
    return object(reader::deref{});
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

      template<typename in_t>
      using converted_t = std::invoke_result_t<converter_t, in_t>;

      template<typename target_t = to_t>
      constexpr decltype(auto) operator()(auto&& val) const
        requires 
          std::assignable_from<target_t&, converted_t<decltype(val)>>
          || concepts::castable_to<converted_t<decltype(val)>, target_t>
      {
        if constexpr(std::assignable_from<target_t&, converted_t<decltype(val)>>)
        {
          return converter_(FWD(val));
        }
        else
        {
          return static_cast<target_t>(converter_(FWD(val)));
        }
      }

      converter_t& converter_;
    };
  }

  namespace operators
  {
    template<typename to_t, typename converter_t>
    using explicit_cast = converter::explicit_cast<to_t, converter_t>;

    struct assign
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<typename lhs_t, typename rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<lhs_t, converter_t>>
        requires concepts::assignable_from_converted<lhs_t&, rhs_t, cast_t>
      constexpr decltype(auto) operator()(lhs_t& lhs, rhs_t&& rhs, converter_t converter = {}) const
      {
        return lhs = cast_t(converter)(FWD(rhs));
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<concepts::range lhs_t, concepts::range rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
        requires 
          (!concepts::assignable_from_converted<lhs_t&, rhs_t, explicit_cast<lhs_t, converter_t>>)
          && concepts::assignable_from_converted<traits::range_value_t<lhs_t>&, traits::range_value_t<rhs_t>, cast_t>
      constexpr decltype(auto) operator()(lhs_t& lhs, rhs_t&& rhs, converter_t converter = {}) const
      {
        using container_iterator_t = std::decay_t<decltype(std::begin(rhs))>;
        using iterator_t = std::conditional_t<
            std::is_rvalue_reference_v<decltype(rhs)>, 
            std::move_iterator<container_iterator_t>, 
            container_iterator_t
          >;

        if constexpr(concepts::resizable<lhs_t>)
        {
          lhs.resize(rhs.size());
        }

        const auto size = std::min(lhs.size(), rhs.size());
        std::for_each(iterator_t{std::begin(rhs)}, iterator_t{std::begin(rhs) + size}, 
          [this, lhsItr = std::begin(lhs), &converter](auto&& rhs) mutable {
            this->operator()(*lhsItr++, FWD(rhs), converter);
          });

        return FWD(lhs);
      }
    };

    struct equal
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<typename lhs_t, typename rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<lhs_t, converter_t>>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {}) const
        requires concepts::equality_comparable_with_converted<lhs_t, rhs_t, cast_t>
      {
        return FWD(lhs) == cast_t(converter)(FWD(rhs));
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<concepts::range lhs_t, concepts::range rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
        requires 
          (!concepts::equality_comparable_with_converted<lhs_t&, rhs_t, explicit_cast<lhs_t, converter_t>>)
          && concepts::equality_comparable_with_converted<traits::range_value_t<lhs_t>, traits::range_value_t<rhs_t>, cast_t>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
      {
        const auto size = std::min(lhs.size(), rhs.size());
        const auto& [rhsItr, lhsItr] = std::mismatch(std::begin(rhs), std::begin(rhs) + size, std::begin(lhs), 
          [this, &converter](const auto& rhs, const auto& lhs){
            return this->operator()(lhs, rhs, converter);
          });

        (void)rhsItr;
        constexpr auto sizeShouldMatch = concepts::resizable<lhs_t>;
        return lhsItr == std::end(lhs) && (sizeShouldMatch ? lhs.size() == rhs.size() : true);
      }
    };
  }

  template<concepts::adapter lhs_adapt_t, concepts::adapter rhs_adapt_t, typename converter_t = converter::identity>
  struct mapping
  {
    using lhs_adapter_t = lhs_adapt_t;
    using rhs_adapter_t = rhs_adapt_t;

    constexpr explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter = {}):
      lhsAdapter_(std::move(lhsAdapter)),
      rhsAdapter_(std::move(rhsAdapter)),
      converter_(std::move(converter))
    {}

    template<typename operator_t, DIR_DECL(direction) dir, concepts::adaptable<lhs_adapter_t> lhs_t, concepts::adaptable<rhs_adapter_t> rhs_t>
      requires concepts::executable_with<dir, operator_t, std::invoke_result_t<lhs_adapter_t, lhs_t>, std::invoke_result_t<rhs_adapter_t, rhs_t>, converter_t>
    constexpr decltype(auto) exec(lhs_t&& lhs, rhs_t&& rhs) const 
    {
      constexpr operator_t op;
      if constexpr(dir == direction::rhs_to_lhs)
        return op(lhsAdapter_(FWD(lhs)), rhsAdapter_(FWD(rhs)), converter_);
      else
        return op(rhsAdapter_(FWD(rhs)), lhsAdapter_(FWD(lhs)), converter_);
    }

    template<DIR_DECL(direction) dir>
    constexpr void assign(auto&& lhs, auto&& rhs) const
      requires requires(mapping m){ m.exec<operators::assign, DIR_READ(dir)>(FWD(lhs), FWD(rhs)); }
    {
      (void)exec<operators::assign, dir>(FWD(lhs), FWD(rhs));
    }

    constexpr bool equal(auto& lhs, auto& rhs) const
      requires requires(mapping m){ m.exec<operators::equal, direction::rhs_to_lhs>(lhs, rhs); }
    {
      return exec<operators::equal, direction::rhs_to_lhs>(FWD(lhs), FWD(rhs));
    }

    template<concepts::adaptable<lhs_adapter_t> lhs_t, concepts::adaptable<rhs_adapter_t> rhs_t = typename rhs_adapter_t::object_decay_t>
    constexpr auto operator()(lhs_t&& lhs) const
      requires requires(mapping m, lhs_t l, rhs_t r){ m.assign<direction::lhs_to_rhs>(l, r); }
    {
      rhs_t rhs;
      assign<direction::lhs_to_rhs>(FWD(lhs), rhs);
      return rhs;
    }

    template<concepts::adaptable<rhs_adapter_t> rhs_t, concepts::adaptable<lhs_adapter_t> lhs_t = typename lhs_adapter_t::object_decay_t>
      requires (!concepts::adaptable<lhs_t, rhs_adapter_t>)
    constexpr auto operator()(rhs_t&& rhs) const
      requires requires(mapping m, lhs_t l, rhs_t r){ m.assign<direction::rhs_to_lhs>(l, r); }
    {
      lhs_t lhs;
      assign<direction::rhs_to_lhs>(lhs, FWD(rhs));
      return lhs;
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

  template<typename... mapping_ts>
  struct mapping_table
  {
    using lhs_unique_types = traits::unique_derived_types_t<std::remove_pointer_t<std::decay_t<typename mapping_ts::lhs_adapter_t::object_t>>...>;
    using rhs_unique_types = traits::unique_derived_types_t<std::remove_pointer_t<std::decay_t<typename mapping_ts::rhs_adapter_t::object_t>>...>;

    constexpr explicit mapping_table(mapping_ts... mappings):
      mappings_(std::move(mappings)...)
    {
    }

    template<DIR_DECL(direction) dir, typename lhs_t, typename rhs_t>
      requires (concepts::mappable<mapping_ts, operators::assign, dir, lhs_t, rhs_t> || ...)
    constexpr void assign(lhs_t&& lhs, rhs_t&& rhs) const
    {
      for_each([&lhs, &rhs](auto&& map){
        using mapping_t = decltype(map);
        if constexpr(concepts::mappable<mapping_t, operators::assign, dir, lhs_t, rhs_t>)
        {
          map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        }
        return true;
      }, mappings_);
    }

    template<typename lhs_t, typename rhs_t>
      requires (concepts::mappable<mapping_ts, operators::equal, direction::rhs_to_lhs, lhs_t, rhs_t> || ...)
    constexpr bool equal(const lhs_t& lhs, const rhs_t& rhs) const
    {
      return for_each([&lhs, &rhs](auto&& map) -> bool{
        using mapping_t = decltype(map);
        if constexpr(concepts::mappable<mapping_t, operators::equal, direction::rhs_to_lhs, lhs_t, rhs_t>)
        {
          return map.equal(lhs, rhs);
        }
        return true;
      }, mappings_);
    }

    template<typename lhs_t, typename result_t = rhs_unique_types>
      requires (concepts::adapted_type_known<typename mapping_ts::rhs_adapter_t> || ...) &&
        (
         traits::adaptable_count_v<lhs_t, typename mapping_ts::lhs_adapter_t...>
         >
         traits::adaptable_count_v<lhs_t, typename mapping_ts::rhs_adapter_t...>
        )
    constexpr auto operator()(lhs_t&& lhs) const
    {
      result_t rets;

      for_each([&](auto&& rhs) -> bool{
        using rhs_t = decltype(rhs);
        if constexpr((concepts::mappable<mapping_ts, operators::assign, direction::lhs_to_rhs, lhs_t, rhs_t> || ...))
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
      requires (concepts::adapted_type_known<typename mapping_ts::lhs_adapter_t> || ...) &&
        (
         traits::adaptable_count_v<rhs_t, typename mapping_ts::lhs_adapter_t...>
         <
         traits::adaptable_count_v<rhs_t, typename mapping_ts::rhs_adapter_t...>
        )
    constexpr auto operator()(rhs_t&& rhs) const
    {
      result_t rets;

      for_each([&](auto&& lhs) -> bool{
        using lhs_t = decltype(lhs);
        if constexpr((concepts::mappable<mapping_ts, operators::assign, direction::rhs_to_lhs, lhs_t, rhs_t> || ...))
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
