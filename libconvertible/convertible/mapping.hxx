#pragma once

#include <convertible/concepts.hxx>
#include <convertible/operators.hxx>
#include <convertible/readers.hxx>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
  template<concepts::adapter _lhs_adapter_t, concepts::adapter _rhs_adapter_t,
           typename _converter_t = converter::identity>
  struct mapping
  {
    using lhs_adapter_t = _lhs_adapter_t;
    using rhs_adapter_t = _rhs_adapter_t;
    using converter_t   = _converter_t;

    constexpr explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter,
                               converter_t converter = {})
      : lhsAdapter_(std::move(lhsAdapter))
      , rhsAdapter_(std::move(rhsAdapter))
      , converter_(std::move(converter))
    {}

    template<direction dir>
    constexpr void
    assign(concepts::adaptable<lhs_adapter_t> auto&& lhs,
           concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires (lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter,
                         converter_t converter) {
                 operators::assign{}.template operator()<dir>(lhsAdapter(FWD(lhs)),
                                                              rhsAdapter(FWD(rhs)), converter);
               }
    {
      if (dir == direction::lhs_to_rhs && !lhsAdapter_.enabled(FWD(lhs)))
      {
        return;
      }
      if (dir == direction::rhs_to_lhs && !rhsAdapter_.enabled(FWD(rhs)))
      {
        return;
      }
      operators::assign{}.template operator()<dir>(lhsAdapter_(FWD(lhs)), rhsAdapter_(FWD(rhs)),
                                                   converter_);
    }

    template<direction dir = direction::rhs_to_lhs>
    constexpr auto
    equal(concepts::adaptable<lhs_adapter_t> auto&& lhs,
          concepts::adaptable<rhs_adapter_t> auto&& rhs) const -> bool
      requires requires (lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter,
                         converter_t converter) {
                 operators::equal{}.template operator()<dir>(lhsAdapter(FWD(lhs)),
                                                             rhsAdapter(FWD(rhs)), converter);
               }
    {
      bool const lhs_enabled = lhsAdapter_.enabled(FWD(lhs));
      bool const rhs_enabled = rhsAdapter_.enabled(FWD(rhs));

      if (lhs_enabled != rhs_enabled)
      {
        return false;
      }
      else if (!lhs_enabled)
      {
        return true;
      }

      return operators::equal{}.template operator()<dir>(lhsAdapter_(FWD(lhs)),
                                                         rhsAdapter_(FWD(rhs)), converter_);
    }

    template<concepts::adaptable<rhs_adapter_t> rhs_t = typename rhs_adapter_t::adaptee_value_t>
    constexpr auto
    operator()(concepts::adaptable<lhs_adapter_t> auto&& lhs) const
      requires requires (rhs_t& rhs) { this->assign<direction::lhs_to_rhs>(FWD(lhs), rhs); }
    {
      rhs_t rhs = defaulted_rhs();
      assign<direction::lhs_to_rhs>(FWD(lhs), rhs);
      return rhs;
    }

    template<concepts::adaptable<lhs_adapter_t> lhs_t = typename lhs_adapter_t::adaptee_value_t>
    constexpr auto
    operator()(concepts::adaptable<rhs_adapter_t> auto&& rhs) const
      requires requires (lhs_t& lhs) { this->assign<direction::rhs_to_lhs>(lhs, FWD(rhs)); }
    {
      lhs_t lhs = defaulted_lhs();
      assign<direction::rhs_to_lhs>(lhs, FWD(rhs));
      return lhs;
    }

    constexpr auto
    defaulted_lhs() const
    {
      return lhsAdapter_.defaulted_adaptee();
    }

    constexpr auto
    defaulted_rhs() const
    {
      return rhsAdapter_.defaulted_adaptee();
    }

  private:
    lhs_adapter_t lhsAdapter_;
    rhs_adapter_t rhsAdapter_;
    converter_t   converter_;
  };
}

#undef FWD
