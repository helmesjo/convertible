#pragma once

#include <convertible/adapter.hxx>
#include <convertible/common.hxx>
#include <convertible/concepts.hxx>
#include <convertible/mapping.hxx>
#include <convertible/mapping_table.hxx>
#include <convertible/operators.hxx>
#include <convertible/readers.hxx>
#include <convertible/std_concepts_ext.hxx>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
  template<concepts::adapter... adapter_ts>
  constexpr auto
  compose(adapter_ts&&... adapters)
  {
    auto adaptee = std::get<0>(std::tuple{adapters...}).defaulted_adaptee();
    return adapter(adaptee, reader::composed(FWD(adapters)...));
  }

  constexpr auto
  custom(auto&& reader)
  {
    return adapter(FWD(reader));
  }

  template<typename adaptee_t>
  constexpr auto
  custom(auto&& reader, adaptee_t&& adaptee)
    requires concepts::adaptable<adaptee_t, decltype(reader)>
  {
    return adapter(FWD(adaptee), FWD(reader));
  }

  constexpr auto
  custom(auto&& reader, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., custom(FWD(reader)));
  }

  constexpr auto
  identity(concepts::adaptable<reader::identity<>> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)...,
                   reader::identity<std::remove_reference_t<decltype(adaptee)>...>{});
  }

  constexpr auto
  identity(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., identity());
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto
  member(member_ptr_t ptr, concepts::adaptable<reader::member<member_ptr_t>> auto&&... adaptee)
  {
    return adapter<traits::member_class_t<member_ptr_t>, reader::member<member_ptr_t>>(
      FWD(adaptee)..., FWD(ptr));
  }

  template<concepts::member_ptr member_ptr_t>
  constexpr auto
  member(member_ptr_t&& ptr, concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., member(FWD(ptr)));
  }

  template<details::const_value i>
  constexpr auto
  index(concepts::adaptable<reader::index<i>> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::index<i>{});
  }

  template<details::const_value i>
  constexpr auto
  index(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., index<i>());
  }

  constexpr auto
  deref(concepts::adaptable<reader::deref> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::deref{});
  }

  constexpr auto
  deref(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., deref());
  }

  constexpr auto
  maybe(concepts::adaptable<reader::maybe> auto&&... adaptee)
  {
    return adapter(FWD(adaptee)..., reader::maybe{});
  }

  constexpr auto
  maybe(concepts::adapter auto&&... inner)
    requires (sizeof...(inner) > 0)
  {
    return compose(FWD(inner)..., maybe());
  }

  template<typename callback_t, typename tuple_t>
  constexpr bool
  for_each(callback_t&& callback, tuple_t&& pack)
  {
    return std::apply(
      [&](auto&&... args)
      {
        return (FWD(callback)(FWD(args)) && ...);
      },
      FWD(pack));
  }

  template<typename... mapping_ts>
  constexpr auto
  extend(mapping_table<mapping_ts...> table, auto&&... mappings)
  {
    return std::apply(
      [&](auto&&... mappings1)
      {
        return mapping_table(std::forward<mapping_ts>(mappings1)...,
                             std::forward<decltype(mappings)>(mappings)...);
      },
      table.mappings());
  }
}

#undef FWD
