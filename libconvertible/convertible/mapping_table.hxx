#pragma once

#include <convertible/concepts.hxx>
#include <convertible/mapping.hxx>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
  template<concepts::mapping... mapping_ts>
  struct mapping_table
  {
    using lhs_unique_types =
      traits::unique_derived_ts<typename mapping_ts::lhs_adapter_t::adaptee_value_t...>;
    using rhs_unique_types =
      traits::unique_derived_ts<typename mapping_ts::rhs_adapter_t::adaptee_value_t...>;

    constexpr lhs_unique_types
    defaulted_lhs() const
    {
      lhs_unique_types rets;
      for_each(
        [this](auto&& lhs)
        {
          using lhs_t = std::remove_reference_t<decltype(lhs)>;
          for_each(
            [&lhs](auto&& mapping)
            {
              using defaulted_lhs_t = decltype(mapping.defaulted_lhs());
              if constexpr (std::is_same_v<defaulted_lhs_t, lhs_t>)
              {
                lhs = mapping.defaulted_lhs();
              }
              return true;
            },
            mappings_);
          return true;
        },
        rets);
      return rets;
    }

    constexpr rhs_unique_types
    defaulted_rhs() const
    {
      rhs_unique_types rets;
      for_each(
        [this](auto&& rhs)
        {
          using rhs_t = std::remove_reference_t<decltype(rhs)>;
          for_each(
            [&rhs](auto&& mapping)
            {
              using defaulted_rhs_t = decltype(mapping.defaulted_rhs());
              if constexpr (std::is_same_v<defaulted_rhs_t, rhs_t>)
              {
                rhs = mapping.defaulted_rhs();
              }
              return true;
            },
            mappings_);
          return true;
        },
        rets);
      return rets;
    }

    constexpr explicit mapping_table(mapping_ts... mappings)
      : mappings_(std::move(mappings)...)
    {}

    template<direction dir, typename lhs_t, typename rhs_t>
    constexpr void
    assign(lhs_t&& lhs, rhs_t&& rhs) const
      requires (concepts::mappable_assign<mapping_ts, lhs_t, rhs_t, dir> || ...)
    {
      for_each(
        [&lhs, &rhs](concepts::mapping auto&& map)
        {
          if constexpr (concepts::mappable_assign<decltype(map), lhs_t, rhs_t, dir>)
          {
            map.template assign<dir>(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
          }
          return true;
        },
        mappings_);
    }

    template<direction dir = direction::rhs_to_lhs>
    constexpr bool
    equal(auto const& lhs, auto const& rhs) const
      requires (
        concepts::mappable_equal<mapping_ts, decltype(lhs), decltype(rhs), direction::rhs_to_lhs> ||
        ...)
    {
      return for_each(
        [&lhs, &rhs](concepts::mapping auto&& map) -> bool
        {
          if constexpr (concepts::mappable_equal<decltype(map), decltype(lhs), decltype(rhs),
                                                 direction::rhs_to_lhs>)
          {
            return map.equal(lhs, rhs);
          }
          else
          {
            return true;
          }
        },
        mappings_);
    }

    template<typename lhs_t, typename result_t = rhs_unique_types>
      requires (concepts::adaptee_type_known<typename mapping_ts::rhs_adapter_t> || ...) &&
               (traits::adaptable_count_v<lhs_t, typename mapping_ts::lhs_adapter_t...> >
                traits::adaptable_count_v<lhs_t, typename mapping_ts::rhs_adapter_t...>)
    constexpr auto
    operator()(lhs_t&& lhs) const
    {
      auto rets = defaulted_rhs();

      for_each(
        [&](auto&& rhs) -> bool
        {
          if constexpr (requires {
                          {
                            assign<direction::lhs_to_rhs>(std::forward<lhs_t>(lhs), rhs)
                          };
                        })
          {
            assign<direction::lhs_to_rhs>(std::forward<lhs_t>(lhs), rhs);
          }
          return true;
        },
        FWD(rets));

      if constexpr (std::tuple_size_v<result_t> == 1)
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
               (traits::adaptable_count_v<rhs_t, typename mapping_ts::lhs_adapter_t...> <
                traits::adaptable_count_v<rhs_t, typename mapping_ts::rhs_adapter_t...>)
    constexpr auto
    operator()(rhs_t&& rhs) const
    {
      auto rets = defaulted_lhs();

      for_each(
        [&](auto&& lhs) -> bool
        {
          if constexpr (requires {
                          {
                            assign<direction::rhs_to_lhs>(lhs, std::forward<rhs_t>(rhs))
                          };
                        })
          {
            assign<direction::rhs_to_lhs>(lhs, std::forward<rhs_t>(rhs));
          }
          return true;
        },
        FWD(rets));

      if constexpr (std::tuple_size_v<result_t> == 1)
      {
        return std::get<0>(rets);
      }
      else
      {
        return rets;
      }
    }

    constexpr auto
    mappings() const
    {
      return mappings_;
    }

  private:
    std::tuple<mapping_ts...> mappings_;
  };
}

#undef FWD
