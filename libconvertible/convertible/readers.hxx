#pragma once

#include <convertible/common.hxx>
#include <convertible/concepts.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::reader
{
  template<typename adaptee_t = details::any>
  struct identity
  {
    constexpr decltype(auto)
    operator()(auto&& obj) const
      requires std::is_same_v<adaptee_t, details::any>
    {
      return FWD(obj);
    }

    constexpr decltype(auto)
    operator()(auto&& obj) const
      requires std::common_reference_with<adaptee_t, decltype(obj)>
    {
      return FWD(obj);
    }
  };

  template<concepts::member_ptr member_ptr_t>
  struct member
  {
    using class_t = traits::member_class_t<member_ptr_t>;

    constexpr member(member_ptr_t ptr)
      : ptr_(std::move(ptr))
    {}

    template<typename obj_t>
      requires std::derived_from<std::remove_reference_t<obj_t>, class_t>
    constexpr decltype(auto)
    operator()(obj_t&& obj) const
    {
      // MSVC bug: 'FWD(obj).*ptr' causes 'fatal error C1001: Internal compiler error'
      //            when 'obj' is r-value reference (in combination with above 'requires' etc.)
      if constexpr (std::is_member_object_pointer_v<member_ptr_t>)
      {
        return std::forward<traits::like_t<decltype(obj), decltype(obj.*ptr_)>>(obj.*ptr_);
      }
      if constexpr (std::is_member_function_pointer_v<member_ptr_t>)
      {
        return (FWD(obj).*ptr_)();
      }
    }

  private:
    member_ptr_t ptr_;
  };

  template<details::const_value i>
  struct index
  {
    constexpr decltype(auto)
    operator()(concepts::indexable<decltype(i)> auto&& obj) const
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
    constexpr decltype(auto)
    operator()(concepts::dereferencable auto&& obj) const
    {
      return *FWD(obj);
    }
  };

  struct maybe
  {
    constexpr decltype(auto)
    operator()(concepts::dereferencable auto&& obj) const
      requires std::constructible_from<bool, decltype(obj)>
    {
      using object_t = decltype(obj);
      using value_t  = std::remove_cvref_t<decltype(*obj)>;

      if constexpr (std::is_assignable_v<object_t, value_t>)
      {
        if (!enabled(FWD(obj)))
        {
          FWD(obj) = value_t{};
        }
      }
      return FWD(obj);
    }

    constexpr bool
    enabled(concepts::dereferencable auto&& obj) const
      requires std::constructible_from<bool, decltype(obj)>
    {
      return bool{FWD(obj)};
    }
  };

  template<concepts::adapter... adapter_ts>
  struct composed
  {
  private:
    using adapters_t = std::tuple<adapter_ts...>;

    template<typename arg_t, typename head_t, typename... tail_t>
    static constexpr bool
    is_composable()
    {
      if constexpr (sizeof...(tail_t) == 0)
      {
        return concepts::adaptable<arg_t, head_t>;
      }
      else if constexpr (is_composable<arg_t, head_t>())
      {
        return is_composable<traits::adapted_t<head_t, arg_t>, tail_t...>();
      }
      else
      {
        return false;
      }
    }

    static constexpr decltype(auto)
    compose(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
      requires (is_composable<decltype(arg), decltype(head), decltype(tail)...>())
    {
      if constexpr (sizeof...(tail) == 0)
      {
        return FWD(head)(FWD(arg));
      }
      else
      {
        return compose(FWD(head)(FWD(arg)), FWD(tail)...);
      }
    }

    static constexpr bool
    enabled_impl(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
      requires (is_composable<decltype(arg), decltype(head), decltype(tail)...>())
    {
      if constexpr (sizeof...(tail) == 0)
      {
        if constexpr (requires { FWD(head).enabled(FWD(arg)); })
        {
          return FWD(head).enabled(FWD(arg));
        }
        else
        {
          return true;
        }
      }
      else if (enabled_impl(FWD(arg), FWD(head)))
      {
        return enabled_impl(compose(FWD(arg), FWD(head)), FWD(tail)...);
      }
      else
      {
        return false;
      }
    }

    adapters_t adapters_;

  public:
    constexpr composed(adapter_ts... adapters)
      : adapters_(std::move(adapters)...)
    {}

    template<typename arg_t>
    constexpr decltype(auto)
    operator()(arg_t&& arg) const
      requires requires () { compose(FWD(arg), std::declval<adapter_ts>()...); }
    {
      return std::apply(
        [&arg](auto&&... adapters) -> decltype(auto)
        {
          return compose(std::forward<arg_t>(arg), FWD(adapters)...);
        },
        adapters_);
    }

    constexpr bool
    enabled(auto&& arg) const
      requires requires { enabled_impl(FWD(arg), std::declval<adapter_ts>()...); }
    {
      using arg_t = decltype(arg);
      return std::apply(
        [&arg](auto&&... adapters) -> bool
        {
          return enabled_impl(std::forward<arg_t>(arg), FWD(adapters)...);
        },
        adapters_);
    }
  };
}

#undef FWD
