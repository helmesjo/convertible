#pragma once

#include <convertible/concepts.hxx>
#include <convertible/converters.hxx>

#include <algorithm>
#include <functional>
#include <iterator>
#include <tuple>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::operators
{
  template<typename to_t, typename converter_t>
  using explicit_cast = converter::explicit_cast<std::remove_reference_t<to_t>, converter_t>;

  template<concepts::associative_container container_t, std::common_reference_with<traits::mapped_value_t<container_t>> mapped_forward_t>
  struct associative_inserter
  {
    using key_t = typename container_t::key_type;
    using value_t = typename container_t::value_type;
    using reference_t = typename container_t::reference;
    using const_reference_t = typename container_t::const_reference;
    using mapped_value_t = traits::mapped_value_t<container_t>;

    container_t& cont_;
    std::insert_iterator<container_t> inserter_;
    std::conditional_t<concepts::mapping_container<container_t>, key_t, value_t> key_;

    template<std::common_reference_with<key_t> _key_t, typename _mapped_t>
    associative_inserter(concepts::associative_container auto&& cont, const std::pair<_key_t, _mapped_t>& pair)
    : cont_(cont),
      inserter_(cont, std::begin(cont)),
      key_(pair.first)
    {}

    associative_inserter(concepts::associative_container auto&& cont, const key_t& key)
    : cont_(cont),
      inserter_(cont, std::begin(cont)),
      key_(key)
    {}

    operator mapped_forward_t()
      requires concepts::mapping_container<container_t>
    {
      if constexpr(std::is_const_v<container_t>)
      {
        return std::forward<mapped_forward_t>(cont_.at(key_));
      }
      else
      {
        return std::forward<mapped_forward_t>(cont_[key_]);
      }
    }

    operator const mapped_value_t&() const
      requires concepts::mapping_container<container_t>
    {
      return cont_.find(key_)->second;
    }

    operator const mapped_value_t&() const
      requires (!concepts::mapping_container<container_t>)
    {
      return *cont_.find(key_);
    }

    auto& operator=(auto&& value)
      requires requires { this->inserter_ = { this->key_, FWD(value) }; }
    {
      inserter_ = { key_, FWD(value) };
      return *this;
    }

    auto& operator=(auto&& value)
      requires requires { this->inserter_ = FWD(value); }
    {
      inserter_ = FWD(value);
      return *this;
    }
  };
  template<concepts::associative_container cont_t>
  associative_inserter(cont_t&& cont, auto&&)
    -> associative_inserter<std::remove_reference_t<decltype(cont)>, traits::like_t<decltype(cont), traits::mapped_value_t<cont_t>>>;

  template<direction dir>
  inline constexpr decltype(auto) ordered_lhs_rhs(auto&& lhs, auto&& rhs)
  {
    if constexpr(dir == direction::rhs_to_lhs)
      return std::forward_as_tuple(FWD(lhs), FWD(rhs));
    else
      return std::forward_as_tuple(FWD(rhs), FWD(lhs));
  }

  namespace details
  {
    template<
      direction dir,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    concept assignable_by_conversion = 
      (concepts::assignable_from_converted<dir, lhs_t, rhs_t, explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>> ||
      requires (lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter){
        converter.template assign<dir>(FWD(lhs), FWD(rhs));
      });

    template<
      direction dir,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    concept equality_comparable_with_converted = 
      (concepts::equality_comparable_with_converted<dir, lhs_t, rhs_t, explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>> ||
      requires (lhs_t&& lhs, rhs_t&& rhs, converter_t converter){
        converter.template equal<dir>(FWD(lhs), FWD(rhs));
      });

    template<
      direction dir,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    constexpr auto equal_dummy(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      -> std::enable_if_t<
        equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), converter_t>,
        bool
    >;

    template<
      direction dir,
      concepts::sequence_container lhs_t,
      concepts::sequence_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr auto equal_dummy(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      -> std::enable_if_t<
        (!requires{ equal_dummy<dir>(FWD(lhs), FWD(rhs), converter); }) &&
        requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
        {
          equal_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
        },
        bool
    >;

    template<
      direction dir,
      concepts::associative_container lhs_t,
      concepts::associative_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr auto equal_dummy(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      -> std::enable_if_t<
        (!requires{ equal_dummy<dir>(FWD(lhs), FWD(rhs), converter); }) &&
        requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
        {
          equal_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
        },
        bool
    >;

    template<
      direction dir,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    constexpr bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      requires equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), converter_t>
    {
      if constexpr(concepts::mapping<converter_t>)
      {
        return converter.template equal<dir>(FWD(lhs), FWD(rhs));
      }
      else
      {
        using cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>;
        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
        return FWD(to) == cast_t(converter)(FWD(from));
      }
    }

    template<
      direction dir,
      concepts::sequence_container lhs_t,
      concepts::sequence_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      requires (!requires{ equal<dir>(FWD(lhs), FWD(rhs), converter); })
            && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
               {
                 equal_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
      if constexpr(!concepts::resizable_container<std::remove_cvref_t<decltype(to)>>
                 && concepts::resizable_container<std::remove_cvref_t<decltype(from)>>)
      {
        if(to.size() > from.size())
          return false;
      }
      if constexpr(!concepts::resizable_container<std::remove_cvref_t<decltype(from)>>
                 && concepts::resizable_container<std::remove_cvref_t<decltype(to)>>)
      {
        if(to.size() < from.size())
          return false;
      }
      if constexpr( concepts::resizable_container<std::remove_cvref_t<decltype(from)>>
                 && concepts::resizable_container<std::remove_cvref_t<decltype(to)>>)
      {
        if(to.size() != from.size())
          return false;
      }
      (void)to;
      (void)from;

      return std::equal(std::cbegin(lhs), std::cend(lhs), std::cbegin(rhs),
        [&converter](auto&& lhs, auto&& rhs){
          return equal<dir>(
                   FWD(lhs),
                   FWD(rhs),
                   converter
                 );
        });
    }

    template<
      direction dir,
      concepts::associative_container lhs_t,
      concepts::associative_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {})
      requires (!requires{ equal<dir>(FWD(lhs), FWD(rhs), converter); })
            && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
               {
                 equal_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      return std::equal(std::cbegin(lhs), std::cend(lhs), std::cbegin(rhs),
        [&converter](auto&& lhs, auto&& rhs){
          // key-value pair (map etc.)
          if constexpr(concepts::mapping_container<rhs_t>)
            return lhs.first == rhs.first &&
                    equal<dir>(
                      std::forward_like<decltype(lhs)>(lhs.second),
                      std::forward_like<decltype(lhs)>(rhs.second),
                      converter
                    );
          // value (set etc.)
          else
            return equal<dir>(FWD(lhs), FWD(rhs), converter);
        });
    }
  }

  struct assign
  {
    template<
      direction dir = direction::rhs_to_lhs,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      requires details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>;

    template<
      direction dir = direction::rhs_to_lhs,
      concepts::sequence_container lhs_t,
      concepts::sequence_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      requires (!details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>)
            && requires(const assign& assigner, traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
               {
                 assigner.template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               };

    template<
      direction dir = direction::rhs_to_lhs,
      concepts::associative_container lhs_t,
      concepts::associative_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      requires (!details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>)
            && requires(const assign& assigner, traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
               {
                 assigner.template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               };
  };

  template<
    direction dir,
    typename lhs_t,
    typename rhs_t,
    typename converter_t
  >
  constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    requires details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>
  {
    auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
    if constexpr(requires{ converter.template assign<dir>(FWD(lhs), FWD(rhs)); })
    {
      (void)from;
      converter.template assign<dir>(FWD(lhs), FWD(rhs));
    }
    else
    {
      using cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>;
      FWD(to) = cast_t(converter)(FWD(from));
    }
    return FWD(to);
  }

  template<
    direction dir,
    concepts::sequence_container lhs_t,
    concepts::sequence_container rhs_t,
    typename converter_t
  >
  constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    requires (!details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>)
          && requires(const assign& assigner, traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
             {
               assigner.template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
             }
  {
    // 1. figure out 'from' & 'to'
    // 2. if 'to' is resizeable: to.resize(from.size())
    // 3. iterate values
    // 4. call assign with lhs & rhs respective range values

    auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));

    if constexpr(concepts::resizable_container<decltype(to)>)
    {
      to.resize(from.size());
    }

    const auto size = std::min(lhs.size(), rhs.size());
    auto end = std::begin(rhs);
    std::advance(end, size);
    std::for_each(std::begin(rhs), end,
      [this, lhsItr = std::begin(lhs), &converter](auto&& rhsElem) mutable {
        this->template operator()<dir>(
          std::forward<traits::range_value_forwarded_t<decltype(lhs)>>(*lhsItr++),
          std::forward<traits::range_value_forwarded_t<decltype(rhs)>>(FWD(rhsElem)),
          converter
        );
      }
    );

    return FWD(to);
  }

  template<
    direction dir,
    concepts::associative_container lhs_t,
    concepts::associative_container rhs_t,
    typename converter_t
  >
  constexpr traits::lhs_t<dir, lhs_t&&, rhs_t&&> assign::operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter) const
    requires (!details::assignable_by_conversion<dir, decltype(lhs), decltype(rhs), converter_t>)
          && requires(const assign& assigner, traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
             {
               assigner.template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
             }
  {
      // 1. figure out 'from', and use that as 'key range'
      // 2. clear 'to'
      // 3. iterate keys
      // 4. create associative_inserter for lhs & rhs using the key
      // 5. call assign with lhs & rhs mapped value respectively (indirectly using inserter)

      auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
      to.clear();
      std::for_each(std::begin(from), std::end(from),
        [this, &lhs, &rhs, &converter](auto&& key) mutable {
          auto&& [toElem, _] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
          associative_inserter(FWD(toElem), key) =
            this->template operator()<dir>(
              std::forward<traits::mapped_value_forwarded_t<lhs_t>>(associative_inserter(FWD(lhs), key)),
              std::forward<traits::mapped_value_forwarded_t<rhs_t>>(associative_inserter(FWD(rhs), key)),
              converter
            );
        }
      );

      return FWD(to);
  }


  struct equal
  {
    template<
      direction dir = direction::rhs_to_lhs,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity
    >
    constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
      requires requires{ details::equal<dir>(FWD(lhs), FWD(rhs), converter); }
    {
      return details::equal<dir>(FWD(lhs), FWD(rhs), converter);
    }
  };
}

#undef FWD
