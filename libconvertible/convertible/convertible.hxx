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
  }

  template<typename obj_t, typename reader_t>
  struct object;

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
      struct is_adapter<object<arg_ts...>>: std::true_type {};

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
  }

  namespace concepts
  {
    template<typename T>
    concept member_ptr = std::is_member_pointer_v<T>;

    template<typename T>
    concept indexable = requires(T t)
    {
      t[0];
    };

    template<typename T>
    concept dereferencable = requires(T t)
    {
      *t;
      requires !std::same_as<void, decltype(*t)>;
    };

    template<typename T>
    concept reference = std::is_reference_v<T>;

    template<typename adaptee_t, typename reader_t>
    concept readable = requires(reader_t reader, adaptee_t&& adaptee)
    {
      { reader(FWD(adaptee)) };
      requires !std::same_as<void, decltype(reader(FWD(adaptee)))>;
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

    template<typename mapping_t, DIR_DECL(direction) dir, typename operator_t, typename lhs_t, typename rhs_t>
    concept mappable = requires(mapping_t m, lhs_t l, rhs_t r)
    {
      { m.template exec<dir, operator_t>(l, r) };
    };

    template<DIR_DECL(direction) dir, typename callable_t, typename arg1_t, typename arg2_t, typename converter_t>
    concept executable_with = requires(callable_t callable, traits::lhs_t<dir, arg1_t, arg2_t> l, traits::rhs_t<dir, arg1_t, arg2_t> r, converter_t converter)
    {
      { callable(l, r, converter) };
    };

    template<typename lhs_t, typename rhs_t, typename converter_t>
    concept assignable_from_converted = std::is_assignable_v<lhs_t, traits::converted_t<converter_t, rhs_t>>;

    template<typename lhs_t, typename rhs_t, typename converter_t>
    concept equality_comparable_with_converted = requires(std::remove_reference_t<lhs_t>& lhs, traits::converted_t<converter_t, rhs_t> rhs)
    {
      { lhs == rhs } -> std::same_as<bool>;
    };

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

    template<concepts::adapter adapter_t, typename adaptee_t>
    using adapted_t = converted_t<adapter_t, adaptee_t>;
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
        requires std::same_as<std::remove_reference_t<decltype(obj)>, adaptee_t>
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
        // Workaround for MSVC bug: 'FWD(obj).*ptr' causes 'fatal error C1001: Internal compiler error'
        //                          when 'obj' is r-value reference (in combination with above 'requires' etc.)
        if constexpr(std::is_member_object_pointer_v<member_ptr_t>)
          return obj.*ptr_;
        if constexpr(std::is_member_function_pointer_v<member_ptr_t>)
          return (obj.*ptr_)();
      }

      member_ptr_t ptr_;
    };

    template<std::size_t i>
    struct index
    {
      constexpr decltype(auto) operator()(concepts::indexable auto&& obj) const
      {
        return FWD(obj)[i];
      }
    };

    struct deref
    {
      constexpr decltype(auto) operator()(concepts::dereferencable auto&& obj) const
      {
        return *FWD(obj);
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
  struct object
  {
    using adaptee_t = _adaptee_t;
    using adaptee_value_t = std::remove_reference_t<adaptee_t>;

    constexpr object() = default;
    constexpr object(const object&) = default;
    constexpr object(object&&) = default;
    constexpr explicit object(adaptee_t adaptee, reader_t reader)
      : reader_(FWD(reader)),
        adaptee_(adaptee)
    {
    }
    constexpr explicit object(reader_t reader)
      : reader_(FWD(reader))
    {
    }

    constexpr decltype(auto) operator()(auto&& obj) const
      requires std::invocable<reader_t, decltype(obj)>
    {
      using obj_t = decltype(obj);
      if constexpr (!std::is_rvalue_reference_v<obj_t> || std::is_pointer_v<std::remove_reference_t<obj_t>>)
      {
        return reader_(FWD(obj));
      }
      else
      {
        return std::move(reader_(FWD(obj)));
      }
    }

    constexpr auto defaulted_adaptee() const
      requires (!std::is_same_v<adaptee_t, details::any>)
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
    return object(adaptee, reader::composed(FWD(adapters)...));
  }

  template<typename adaptee_t = details::any>
  constexpr auto custom(auto&& reader, concepts::readable<decltype(reader)> auto&&... adaptee)
  {
    return object<adaptee_t, decltype(reader)>(FWD(adaptee)..., FWD(reader));
  }

  template<typename adaptee_t = details::any>
  constexpr auto custom(auto&& reader, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(custom(FWD(reader)), FWD(inner)...);
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t ptr, concepts::readable<reader::member<member_ptr_t>> auto&&... adaptee)
  {
    return object<traits::member_class_t<member_ptr_t>, reader::member<member_ptr_t>>(FWD(adaptee)..., FWD(ptr));
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t&& ptr, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(member(FWD(ptr)), FWD(inner)...);
  }

  template<std::size_t i>
  constexpr auto index(concepts::readable<reader::index<i>> auto&&... adaptee)
  {
    return object(FWD(adaptee)..., reader::index<i>{});
  }

  template<std::size_t i>
  constexpr auto index(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(index<i>(), FWD(inner)...);
  }

  constexpr auto deref(concepts::readable<reader::deref> auto&&... adaptee)
  {
    return object(FWD(adaptee)..., reader::deref{});
  }

  constexpr auto deref(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(deref(), FWD(inner)...);
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

      constexpr decltype(auto) operator()(auto&& val) const
        requires std::is_assignable_v<to_t&, converted_t<decltype(val)>>
              || concepts::castable_to<converted_t<decltype(val)>, to_t>
      {
        if constexpr(std::is_assignable_v<to_t&, converted_t<decltype(val)>>)
        {
          return converter_(FWD(val));
        }
        else
        {
          return static_cast<to_t>(converter_(FWD(val)));
        }
      }

      converter_t& converter_;
    };
  }

  namespace operators
  {
    template<typename to_t, typename converter_t>
    using explicit_cast = converter::explicit_cast<std::remove_reference_t<to_t>, converter_t>;

    struct assign
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<typename lhs_t, typename rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<lhs_t, converter_t>>
        requires concepts::assignable_from_converted<lhs_t, rhs_t, cast_t>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      {
        return lhs = cast_t(converter)(FWD(rhs));
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<concepts::range lhs_t, concepts::range rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
        requires 
          (!concepts::assignable_from_converted<lhs_t&, rhs_t, explicit_cast<lhs_t, converter_t>>)
          && concepts::assignable_from_converted<traits::range_value_t<lhs_t>&, traits::range_value_t<rhs_t>, cast_t>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      {
        using container_iterator_t = std::remove_reference_t<decltype(std::begin(rhs))>;
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
        requires concepts::equality_comparable_with_converted<lhs_t, rhs_t, cast_t>
      constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {}) const
      {
        return FWD(lhs) == cast_t(converter)(FWD(rhs));
      }

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<concepts::range lhs_t, concepts::range rhs_t, typename converter_t = converter::identity, typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
        requires 
          (!concepts::equality_comparable_with_converted<lhs_t, rhs_t, explicit_cast<lhs_t, converter_t>>)
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

    template<DIR_DECL(direction) dir, typename operator_t, concepts::adaptable<lhs_adapter_t> lhs_t, concepts::adaptable<rhs_adapter_t> rhs_t>
      requires concepts::executable_with<dir, operator_t, traits::adapted_t<lhs_adapter_t, lhs_t>, traits::adapted_t<rhs_adapter_t, rhs_t>, converter_t>
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
      requires requires(mapping m){ m.exec<DIR_READ(dir), operators::assign>(FWD(lhs), FWD(rhs)); }
    {
      (void)exec<dir, operators::assign>(FWD(lhs), FWD(rhs));
    }

    template<DIR_DECL(direction) dir = direction::rhs_to_lhs>
    constexpr bool equal(auto&& lhs, auto&& rhs) const
      requires requires(mapping m){ m.exec<dir, operators::equal>(lhs, rhs); }
    {
      return exec<dir, operators::equal>(FWD(lhs), FWD(rhs));
    }

    template<concepts::adaptable<lhs_adapter_t> lhs_t, concepts::adaptable<rhs_adapter_t> rhs_t = typename rhs_adapter_t::adaptee_value_t>
    constexpr auto operator()(lhs_t&& lhs) const
      requires requires(mapping m, lhs_t l, rhs_t r){ m.assign<direction::lhs_to_rhs>(l, r); }
    {
      rhs_t rhs = defaulted_rhs();
      assign<direction::lhs_to_rhs>(FWD(lhs), rhs);
      return rhs;
    }

    template<concepts::adaptable<rhs_adapter_t> rhs_t, concepts::adaptable<lhs_adapter_t> lhs_t = typename lhs_adapter_t::adaptee_value_t>
    constexpr auto operator()(rhs_t&& rhs) const
      requires requires(mapping m, lhs_t l, rhs_t r){ m.assign<direction::rhs_to_lhs>(l, r); }
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

  template<typename... mapping_ts>
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
      requires (concepts::mappable<mapping_ts, dir, operators::assign, lhs_t, rhs_t> || ...)
    constexpr void assign(lhs_t&& lhs, rhs_t&& rhs) const
    {
      for_each([&lhs, &rhs](auto&& map){
        using mapping_t = decltype(map);
        if constexpr(concepts::mappable<mapping_t, dir, operators::assign, lhs_t, rhs_t>)
        {
          map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        }
        return true;
      }, mappings_);
    }

    template<typename lhs_t, typename rhs_t>
      requires (concepts::mappable<mapping_ts, direction::rhs_to_lhs, operators::equal, lhs_t, rhs_t> || ...)
    constexpr bool equal(const lhs_t& lhs, const rhs_t& rhs) const
    {
      return for_each([&lhs, &rhs](auto&& map) -> bool{
        using mapping_t = decltype(map);
        if constexpr(concepts::mappable<mapping_t, direction::rhs_to_lhs, operators::equal, lhs_t, rhs_t>)
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
        if constexpr(requires{ { assign<direction::lhs_to_rhs>(std::forward<lhs_t>(lhs), FWD(rhs)) }; })
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
        if constexpr(requires{ { assign<direction::rhs_to_lhs>(FWD(lhs), std::forward<rhs_t>(rhs)) }; })
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
