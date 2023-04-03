#pragma once

#include <convertible/concepts.hxx>
#include <convertible/common.hxx>
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
            && concepts::castable_to<obj_t, std_ext::like_t<decltype(obj), adaptee_t>>
            && concepts::readable<std_ext::like_t<decltype(obj), adaptee_t>, reader_t>
    {
      return reader_(static_cast<std_ext::like_t<decltype(obj), adaptee_t>>(FWD(obj)));
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

    template<direction dir>
    constexpr void assign(concepts::adaptable<lhs_adapter_t> auto&& lhs, concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter, converter_t converter)
      {
        operators::assign{}.template operator()<dir>(lhsAdapter(FWD(lhs)), rhsAdapter(FWD(rhs)), converter);
      }
    {
      operators::assign{}.template operator()<dir>(lhsAdapter_(FWD(lhs)), rhsAdapter_(FWD(rhs)), converter_);
    }

    template<direction dir = direction::rhs_to_lhs>
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
