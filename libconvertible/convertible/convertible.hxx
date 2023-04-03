#pragma once

#include <convertible/adapter.hxx>
#include <convertible/concepts.hxx>
#include <convertible/common.hxx>
#include <convertible/mapping.hxx>
#include <convertible/operators.hxx>
#include <convertible/readers.hxx>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
  template<concepts::adapter... adapter_ts>
  constexpr auto compose(adapter_ts&&... adapters)
  {
    auto adaptee = std::get<sizeof...(adapters)-1>(std::tuple{adapters...}).defaulted_adaptee();
    return adapter(adaptee, reader::composed(FWD(adapters)...));
  }

  constexpr auto custom(auto&& reader)
  {
    return adapter(FWD(reader));
  }

  template<typename adaptee_t>
  constexpr auto custom(auto&& reader, adaptee_t&& adaptee)
    requires concepts::readable<adaptee_t, decltype(reader)>
  {
    return adapter(FWD(adaptee), FWD(reader));
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

    template<direction dir, typename lhs_t, typename rhs_t>
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

    template<direction dir = direction::rhs_to_lhs>
    constexpr bool equal(const auto& lhs, const auto& rhs) const
      requires (concepts::mappable_equal<mapping_ts, decltype(lhs), decltype(rhs), direction::rhs_to_lhs> || ...)
    {
      return for_each([&lhs, &rhs](concepts::mapping auto&& map) -> bool{
        if constexpr(concepts::mappable_equal<decltype(map), decltype(lhs), decltype(rhs), direction::rhs_to_lhs>)
        {
          return map.equal(lhs, rhs);
        }
        else
        {
          return true;
        }
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
