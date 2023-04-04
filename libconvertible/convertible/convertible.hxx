#pragma once

#include <convertible/adapter.hxx>
#include <convertible/concepts.hxx>
#include <convertible/common.hxx>
#include <convertible/mapping.hxx>
#include <convertible/mapping_table.hxx>
#include <convertible/operators.hxx>
#include <convertible/readers.hxx>

#include <convertible/std_concepts_ext.hxx>

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

  template<std_ext::member_ptr member_ptr_t>
  constexpr auto member(member_ptr_t ptr, concepts::readable<reader::member<member_ptr_t>> auto&&... adaptee)
  {
    return adapter<std_ext::member_class_t<member_ptr_t>, reader::member<member_ptr_t>>(FWD(adaptee)..., FWD(ptr));
  }

  template<std_ext::member_ptr member_ptr_t>
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

  template<typename... mapping_ts>
  constexpr auto extend(mapping_table<mapping_ts...> table, auto&&... mappings)
  {
    return std::apply([&](auto&&... mappings1){
      return mapping_table(std::forward<mapping_ts>(mappings1)..., std::forward<decltype(mappings)>(mappings)...);
    }, table.mappings_);
  }
}

#undef FWD
