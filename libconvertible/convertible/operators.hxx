#pragma once

#include <convertible/concepts.hxx>
#include <convertible/converters.hxx>

#include <algorithm>
#include <iterator>
#include <tuple>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::operators
{
  namespace details
  {
    template<typename to_t, typename converter_t>
    using explicit_cast = converter::explicit_cast<std::remove_reference_t<to_t>, converter_t>;

    template<concepts::associative_container                                 container_t,
             std::common_reference_with<traits::mapped_value_t<container_t>> mapped_forward_t>
    struct associative_inserter
    {
      using key_t             = typename container_t::key_type;
      using value_t           = typename container_t::value_type;
      using reference_t       = typename container_t::reference;
      using const_reference_t = typename container_t::const_reference;
      using mapped_value_t    = traits::mapped_value_t<container_t>;
      using inserter_t        = std::insert_iterator<container_t>;

      template<std::common_reference_with<key_t> _key_t, typename _mapped_t>
      associative_inserter(concepts::associative_container auto&& cont,
                           std::pair<_key_t, _mapped_t> const&    pair)
        : cont_(cont)
        , inserter_(cont, std::begin(cont))
        , key_(pair.first)
      {}

      associative_inserter(concepts::associative_container auto&& cont, key_t const& key)
        : cont_(cont)
        , inserter_(cont, std::begin(cont))
        , key_(key)
      {}

      [[nodiscard]] auto
      has_value() const -> bool
      {
        return cont_.contains(key_);
      }

      operator mapped_forward_t()
        requires (!std::is_const_v<container_t>)
      {
        if constexpr (concepts::mapping_container<container_t>)
        {
          return std::forward<mapped_forward_t>(cont_[key_]);
        }
        else
        {
          return std::forward<mapped_forward_t>(*cont_.find(key_));
        }
      }

      operator mapped_forward_t() const
        requires std::is_const_v<container_t>
      {
        if constexpr (concepts::mapping_container<container_t>)
        {
          return std::forward<mapped_forward_t>(cont_.at(key_));
        }
        else
        {
          return std::forward<mapped_forward_t>(*cont_.find(key_));
        }
      }

      auto
      operator=(auto&& value) -> auto&
        requires concepts::mapping_container<container_t>
      // Workaround bug with apple-clang & using 'this->' in requires clause.
#if defined(__clang__) && defined(__APPLE__)
                 && requires (inserter_t inserter, key_t key) {
                      inserter = {key, FWD(value)};
                    }
#else
                 && requires {
                      this->inserter_ = {this->key_, FWD(value)};
                    }
#endif
      {
        inserter_ = {key_, FWD(value)};
        return *this;
      }

      auto
      operator=(auto&& value) -> auto&
        requires (!concepts::mapping_container<container_t>)
      // Workaround bug with apple-clang & using 'this->' in requires clause.
#if defined(__clang__) && defined(__APPLE__)
                 && requires (inserter_t inserter) { inserter = FWD(value); }
#else
                 && requires { this->inserter_ = FWD(value); }
#endif
      {
        inserter_ = FWD(value);
        return *this;
      }

    private:
      container_t&                                                                 cont_; // NOLINT
      inserter_t                                                                   inserter_;
      std::conditional_t<concepts::mapping_container<container_t>, key_t, value_t> key_;
    };

    template<concepts::associative_container cont_t>
    associative_inserter(cont_t&& cont, auto&&)
      -> associative_inserter<std::remove_reference_t<decltype(cont)>,
                              traits::like_t<decltype(cont), traits::mapped_value_t<cont_t>>>;

    template<direction dir>
    inline constexpr auto
    ordered_lhs_rhs(auto&& lhs, auto&& rhs) -> decltype(auto)
    {
      if constexpr (dir == direction::rhs_to_lhs)
      {
        return std::forward_as_tuple(FWD(lhs), FWD(rhs));
      }
      else
      {
        return std::forward_as_tuple(FWD(rhs), FWD(lhs));
      }
    }

    template<direction dir, typename lhs_t, typename rhs_t,
             typename converter_t = converter::identity>
    concept assignable_with_converted =
      (concepts::assignable_from_converted<
         dir, lhs_t, rhs_t, explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>> ||
       requires (lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter) {
         converter.template assign<dir>(FWD(lhs), FWD(rhs));
       });

    template<direction dir, typename lhs_t, typename rhs_t,
             typename converter_t = converter::identity>
    concept equality_comparable_with_converted =
      (concepts::equality_comparable_with_converted<
         dir, lhs_t, rhs_t, explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>> ||
       requires (lhs_t&& lhs, rhs_t&& rhs, converter_t converter) {
         converter.template equal<dir>(FWD(lhs), FWD(rhs));
       });

    template<typename operator_t, direction dir, typename lhs_t, typename rhs_t,
             typename converter_t>
    concept invocable_with =
    // Workaround required for GCC which fails with a requires-clause referring
    // to self (check if 'this' can be invoked recursively) if (and only if) a template parameter is
    // used.
#ifdef __GNUC__
      (dir == direction::rhs_to_lhs &&
       requires (operator_t&& op, lhs_t lhs, rhs_t rhs, converter_t&& converter) {
         op(FWD(lhs), FWD(rhs), FWD(converter));
       }) ||
      (dir == direction::lhs_to_rhs &&
       requires (operator_t&& op, lhs_t lhs, rhs_t rhs, converter_t&& converter) {
         op(FWD(rhs), FWD(lhs), FWD(converter));
       });
#else
      requires (operator_t&& op, lhs_t&& lhs, rhs_t rhs, converter_t&& converter) {
        op.template operator()<dir>(FWD(lhs), FWD(rhs), FWD(converter));
      };
#endif
  }

  struct assign
  {
    template<direction dir        = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
             typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
      requires details::assignable_with_converted<dir, lhs_t&&, rhs_t&&, converter_t>;

    template<direction dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t,
             concepts::sequence_container rhs_t, typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
      requires (!details::assignable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                    converter_t>) &&
               details::invocable_with<assign, dir, traits::range_value_forwarded_t<lhs_t>,
                                       traits::range_value_forwarded_t<rhs_t>, converter_t>;

    template<direction dir = direction::rhs_to_lhs, concepts::associative_container lhs_t,
             concepts::associative_container rhs_t, typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
      requires (!details::assignable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                    converter_t>) &&
               details::invocable_with<assign, dir, traits::mapped_value_forwarded_t<lhs_t>,
                                       traits::mapped_value_forwarded_t<rhs_t>, converter_t>;
  };

  template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
  constexpr auto
  assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
    requires details::assignable_with_converted<dir, lhs_t&&, rhs_t&&, converter_t>
  {
    auto&& [to, from] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
    if constexpr (requires { converter.template assign<dir>(FWD(lhs), FWD(rhs)); })
    {
      (void)from;
      converter.template assign<dir>(FWD(lhs), FWD(rhs));
    }
    else
    {
      using cast_t = details::explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>;
      FWD(to)      = cast_t(converter)(FWD(from));
    }
    return FWD(to);
  }

  template<direction dir, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
           typename converter_t>
  constexpr auto
  assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
    requires (!details::assignable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                  converter_t>) &&
             details::invocable_with<assign, dir, traits::range_value_forwarded_t<lhs_t>,
                                     traits::range_value_forwarded_t<rhs_t>, converter_t>
  {
    // 1. figure out 'from' & 'to'
    // 2. if 'to' is resizeable: to.resize(from.size())
    // 3. iterate values
    // 4. call assign with lhs & rhs respective range values

    auto&& [to, from] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));

    if constexpr (concepts::resizable_container<decltype(to)>)
    {
      to.resize(from.size());
    }

    auto const size = std::min(lhs.size(), rhs.size());
    auto       end  = std::begin(rhs);
    std::advance(end, size);
    std::for_each(std::begin(rhs), end,
                  [this, lhsItr = std::begin(lhs), &converter](auto&& rhsElem) mutable
                  {
                    this->template operator()<dir>(
                      std::forward<traits::range_value_forwarded_t<decltype(lhs)>>(*lhsItr++),
                      std::forward<traits::range_value_forwarded_t<decltype(rhs)>>(FWD(rhsElem)),
                      converter);
                  });

    return FWD(to);
  }

  template<direction dir, concepts::associative_container lhs_t,
           concepts::associative_container rhs_t, typename converter_t>
  constexpr auto
  assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    -> traits::lhs_t<dir, lhs_t&&, rhs_t&&>
    requires (!details::assignable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                  converter_t>) &&
             details::invocable_with<assign, dir, traits::mapped_value_forwarded_t<lhs_t>,
                                     traits::mapped_value_forwarded_t<rhs_t>, converter_t>
  {
    // 1. figure out 'from', and use that as 'key range'
    // 2. clear 'to'
    // 3. iterate keys
    // 4. create associative_inserter for lhs & rhs using the key
    // 5. call assign with lhs & rhs mapped value respectively (indirectly using inserter)

    auto&& [to, from] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
    to.clear();
    for (decltype(auto) key : FWD(from))
    {
      details::associative_inserter(FWD(to), FWD(key)) = this->template operator()<dir>(
        std::forward<traits::mapped_value_forwarded_t<decltype(lhs)>>(
          details::associative_inserter(FWD(lhs), key)),
        std::forward<traits::mapped_value_forwarded_t<decltype(rhs)>>(
          details::associative_inserter(FWD(rhs), key)),
        converter);
    }

    return FWD(to);
  }

  struct equal
  {
    template<direction dir        = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
             typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter = {}) const
      -> bool
      requires details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                           converter_t>;

    template<direction dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t,
             concepts::sequence_container rhs_t, typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter = {}) const
      -> bool
      requires (!details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                             converter_t>) &&
               details::invocable_with<equal, dir, traits::range_value_forwarded_t<lhs_t>,
                                       traits::range_value_forwarded_t<rhs_t>, converter_t>;

    template<direction dir = direction::rhs_to_lhs, concepts::associative_container lhs_t,
             concepts::associative_container rhs_t, typename converter_t = converter::identity>
    constexpr auto operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter = {}) const
      -> bool
      requires (!details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                             converter_t>) &&
               details::invocable_with<equal, dir, traits::mapped_value_forwarded_t<lhs_t>,
                                       traits::mapped_value_forwarded_t<rhs_t>, converter_t>;
  };

  template<direction dir, typename lhs_t, typename rhs_t, typename converter_t>
  constexpr auto
  equal::operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter) const -> bool
    requires details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                         converter_t>
  {
    if constexpr (concepts::mapping<converter_t>)
    {
      return converter.template equal<dir>(FWD(lhs), FWD(rhs));
    }
    else
    {
      using cast_t      = details::explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>;
      auto&& [to, from] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
      return FWD(to) == cast_t(converter)(FWD(from));
    }
  }

  template<direction dir, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
           typename converter_t>
  constexpr auto
  equal::operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter) const -> bool
    requires (!details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                           converter_t>) &&
             details::invocable_with<equal, dir, traits::range_value_forwarded_t<lhs_t>,
                                     traits::range_value_forwarded_t<rhs_t>, converter_t>
  {
    auto&& [to, from] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
    if constexpr (!concepts::resizable_container<std::remove_cvref_t<decltype(to)>> &&
                  concepts::resizable_container<std::remove_cvref_t<decltype(from)>>)
    {
      if (to.size() > from.size())
      {
        return false;
      }
    }
    if constexpr (!concepts::resizable_container<std::remove_cvref_t<decltype(from)>> &&
                  concepts::resizable_container<std::remove_cvref_t<decltype(to)>>)
    {
      if (to.size() < from.size())
      {
        return false;
      }
    }
    if constexpr (concepts::resizable_container<std::remove_cvref_t<decltype(from)>> &&
                  concepts::resizable_container<std::remove_cvref_t<decltype(to)>>)
    {
      if (to.size() != from.size())
      {
        return false;
      }
    }
    (void)to;
    (void)from;

    return std::equal(std::cbegin(lhs), std::cend(lhs), std::cbegin(rhs),
                      [this, &converter](auto&& lhs, auto&& rhs)
                      {
                        return this->template operator()<dir>(FWD(lhs), FWD(rhs), converter);
                      });
  }

  template<direction dir, concepts::associative_container lhs_t,
           concepts::associative_container rhs_t, typename converter_t>
  constexpr auto
  equal::operator()(lhs_t const& lhs, rhs_t const& rhs, converter_t converter) const -> bool
    requires (!details::equality_comparable_with_converted<dir, lhs_t&&, rhs_t&&,
                                                           converter_t>) &&
             details::invocable_with<equal, dir, traits::mapped_value_forwarded_t<lhs_t>,
                                     traits::mapped_value_forwarded_t<rhs_t>, converter_t>
  {
    // 1. use actual lhs as 'key range'
    // 2. iterate keys
    // 3. check that actual rhs contains key
    // 4. create associative_inserter for lhs & rhs using the key
    // 5. call equal with lhs & rhs mapped value respectively (indirectly using inserter)

    auto const& [actualLhs, actualRhs] = details::ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
    for (auto const& elem : actualLhs)
    {
      if (!details::associative_inserter(FWD(actualRhs), elem).has_value())
      {
        return false;
      }

      if (!this->template operator()<dir>(
            std::forward<traits::mapped_value_forwarded_t<decltype(lhs)>>(
              details::associative_inserter(FWD(lhs), elem)),
            std::forward<traits::mapped_value_forwarded_t<decltype(rhs)>>(
              details::associative_inserter(FWD(rhs), elem)),
            converter))
      {
        return false;
      }
    }
    return true;
  }
}

#undef FWD
