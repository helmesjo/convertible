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
      typename converter_t = converter::identity,
      typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>
    >
    constexpr auto assign_dummy(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      -> std::enable_if_t<
        (concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), cast_t> ||
        requires{ converter.template assign<dir>(FWD(lhs), FWD(rhs)); }),
        traits::lhs_t<dir, decltype(lhs), decltype(rhs)>
    >;

    template<
      direction dir,
      concepts::sequence_container lhs_t,
      concepts::sequence_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr auto assign_dummy(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      -> std::enable_if_t<
        ((!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>) &&
        requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
        {
          assign_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
        }),
        traits::lhs_t<dir, decltype(lhs), decltype(rhs)>
    >;

    template<
      direction dir,
      concepts::associative_container lhs_t,
      concepts::associative_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr auto assign_dummy(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      -> std::enable_if_t<
        ((!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>) &&
        requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
        {
          assign_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
        }),
        traits::lhs_t<dir, decltype(lhs), decltype(rhs)>
    >;

    template<
      direction dir,
      typename lhs_t,
      typename rhs_t,
      typename converter_t = converter::identity,
      typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>
    >
    constexpr decltype(auto) assign(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      requires requires{ assign_dummy<dir, lhs_t, rhs_t, converter_t, cast_t>(FWD(lhs), FWD(rhs), converter); }
    {
      auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
      if constexpr(requires{ converter.template assign<dir>(FWD(lhs), FWD(rhs)); })
      {
        (void)from;
        converter.template assign<dir>(FWD(lhs), FWD(rhs));
      }
      else
      {
        FWD(to) = cast_t(converter)(FWD(from));
      }
      return FWD(to);
    }

    template<
      direction dir,
      concepts::sequence_container lhs_t,
      concepts::sequence_container rhs_t,
      typename converter_t = converter::identity
    >
    constexpr decltype(auto) assign(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      requires (!requires{ assign<dir>(FWD(lhs), FWD(rhs), converter); })
            && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
               {
                 assign_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      // 1. figure out 'from' & 'to'
      // 2. if 'to' is resizeable: to.resize(from.size())
      // 3. iterate values
      // 4. call self with lhs & rhs respective range values

      auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));

      if constexpr(concepts::resizable_container<decltype(to)>)
      {
        to.resize(from.size());
      }

      const auto size = std::min(lhs.size(), rhs.size());
      auto end = std::begin(rhs);
      std::advance(end, size);
      std::for_each(std::begin(rhs), end,
        [lhsItr = std::begin(lhs), &converter](auto&& rhsElem) mutable {
          assign<dir>(
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
      typename converter_t = converter::identity
    >
    constexpr decltype(auto) assign(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
      requires (!requires{ assign<dir>(FWD(lhs), FWD(rhs), converter); })
            && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
               {
                 assign_dummy<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      // 1. figure out 'from', and use that as 'key range'
      // 2. clear 'to'
      // 3. iterate keys
      // 4. create associative_inserter for lhs & rhs using the key
      // 5. call self with lhs & rhs mapped value respectively (indirectly using inserter)

      auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
      to.clear();
      std::for_each(std::begin(from), std::end(from),
        [&lhs, &rhs, &converter](auto&& key) mutable {
          auto lhsInserter = associative_inserter(FWD(lhs), key);
          auto rhsInserter = associative_inserter(FWD(rhs), key);
          using lhs_inserter_forward_t = traits::like_t<decltype(lhs), decltype(lhsInserter)>;
          using rhs_inserter_forward_t = traits::like_t<decltype(rhs), decltype(rhsInserter)>;
          using to_mapped_t = traits::mapped_value_t<traits::lhs_t<dir, decltype(lhs), decltype(rhs)>>;
          using cast_t = explicit_cast<to_mapped_t, converter_t>;
          assign<dir, lhs_inserter_forward_t, rhs_inserter_forward_t, converter_t, cast_t>(
            std::forward<lhs_inserter_forward_t>(lhsInserter),
            std::forward<rhs_inserter_forward_t>(rhsInserter),
            converter
          );
        }
      );

      return FWD(to);
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
    constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
      requires requires{ details::assign<dir>(FWD(lhs), FWD(rhs), converter); }
    {
      return details::assign<dir>(FWD(lhs), FWD(rhs), converter);
    }
  };

  struct equal
  {
    // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
    template<direction dir = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
      typename converter_t = converter::identity,
      typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>
    constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {}) const
      requires (concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), cast_t>
            || requires{ converter.template equal<dir>(FWD(lhs), FWD(rhs)); })
    {
      if constexpr(concepts::mapping<converter_t>)
      {
        return converter.template equal<dir>(FWD(lhs), FWD(rhs));
      }
      else
      {
        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
        return FWD(to) == cast_t(converter)(FWD(from));
      }
    }

    // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
    template<direction dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
      typename converter_t = converter::identity,
      typename cast_t = explicit_cast<traits::range_value_t<lhs_t>, converter_t>>
    constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
      requires (!concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<lhs_t, converter_t>>)
            && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
               {
                 this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      if constexpr(dir == direction::rhs_to_lhs && concepts::resizable_container<lhs_t>)
      {
        if(lhs.size() != rhs.size())
          return false;
      }
      if constexpr(dir == direction::lhs_to_rhs && concepts::resizable_container<rhs_t>)
      {
        if(lhs.size() != rhs.size())
          return false;
      }

      return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
        [this, &converter](const auto& lhs, const auto& rhs){
          return this->template operator()<dir>(lhs, rhs, converter);
        });
    }

    // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
    template<direction dir = direction::rhs_to_lhs, concepts::associative_container lhs_t, concepts::associative_container rhs_t,
      typename converter_t = converter::identity,
      typename cast_t = explicit_cast<traits::mapped_value_t<lhs_t>, converter_t>>
    constexpr decltype(auto) operator()(const lhs_t& lhs, const rhs_t& rhs, converter_t converter = {}) const
      requires (!concepts::equality_comparable_with_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<lhs_t, converter_t>>)
            && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
               {
                 this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
               }
    {
      return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
        [this, &converter](const auto& lhs, const auto& rhs){
          // key-value pair (map etc.)
          if constexpr(concepts::mapping_container<rhs_t>)
            return lhs.first == rhs.first && this->template operator()<dir>(lhs.second, rhs.second, converter);
          // value (set etc.)
          else
            return this->template operator()<dir>(lhs, rhs, converter);
        });
    }
  };
}

#undef FWD
