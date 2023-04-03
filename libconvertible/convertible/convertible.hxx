#pragma once

#include <convertible/concepts.hxx>
#include <convertible/common.hxx>
#include <convertible/readers.hxx>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <version>

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

  namespace converter
  {
    struct identity
    {
      constexpr decltype(auto) operator()(auto&& val) const
      {
        return FWD(val);
      }
    };

    template<typename to_t, typename converter_t>
    struct explicit_cast
    {
      constexpr explicit_cast(converter_t& converter):
        converter_(converter)
      {}

      template<typename arg_t>
      using converted_t = traits::converted_t<converter_t, arg_t>;

      constexpr decltype(auto) operator()(auto&& obj) const
        requires std::is_assignable_v<to_t&, converted_t<decltype(obj)>>
              || concepts::castable_to<converted_t<decltype(obj)>, to_t>
      {
        if constexpr(std::is_assignable_v<to_t&, converted_t<decltype(obj)>>)
        {
          return converter_(FWD(obj));
        }
        else
        {
          return static_cast<to_t>(converter_(FWD(obj)));
        }
      }

      converter_t& converter_;
    };
  }

  namespace operators
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

      operator mapped_forward_t()&
        requires concepts::mapping_container<container_t>
      {
        return std::forward<mapped_forward_t>(cont_[key_]);
      }

      operator mapped_forward_t()&&
        requires concepts::mapping_container<container_t>
      {
        return std::forward<mapped_forward_t>(cont_[key_]);
      }

      operator mapped_value_t&()&
        requires concepts::mapping_container<container_t>
              && (!std::is_same_v<mapped_value_t&, mapped_forward_t>)
      {
        return cont_[key_];
      }

      operator mapped_forward_t()&
        requires (!concepts::mapping_container<container_t>)
      {
        return std::forward<mapped_forward_t>(cont_.at(key_));
      }

      auto& operator=(auto&& value)
        requires requires
                 {
                   this->inserter_ = { this->key_, FWD(value) };
                 }
      {
        inserter_ = { key_, FWD(value) };
        return *this;
      }

      auto& operator=(auto&& value)
        requires requires
                 {
                   this->inserter_ = FWD(value);
                 }
      {
        inserter_ = FWD(value);
        return *this;
      }
    };
    template<concepts::associative_container cont_t>
    associative_inserter(cont_t&& cont, auto&&)
      -> associative_inserter<std::remove_reference_t<decltype(cont)>, std_ext::like_t<decltype(cont), traits::mapped_value_t<cont_t>>>;

    template<direction dir>
    inline constexpr decltype(auto) ordered_lhs_rhs(auto&& lhs, auto&& rhs)
    {
      if constexpr(dir == direction::rhs_to_lhs)
        return std::forward_as_tuple(FWD(lhs), FWD(rhs));
      else
        return std::forward_as_tuple(FWD(rhs), FWD(lhs));
    }

    struct assign
    {
      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<direction dir = direction::rhs_to_lhs, typename lhs_t, typename rhs_t,
        typename converter_t = converter::identity,
        typename cast_t = explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), cast_t>
              || requires{ converter.template assign<dir>(FWD(lhs), FWD(rhs)); })
      {
        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));
        if constexpr(concepts::mapping<converter_t>)
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

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<direction dir = direction::rhs_to_lhs, concepts::sequence_container lhs_t, concepts::sequence_container rhs_t,
        typename converter_t = converter::identity>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>)
              && requires(traits::range_value_forwarded_t<lhs_t> lhsElem, traits::range_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
                 }
      {
        // 1. figure out 'from' & 'to'
        // 2. if 'to' is resizeable: to.resize(from.size())
        // 3. iterate values
        // 4. call self with lhs & rhs respective range values

        auto&& [to, from] = ordered_lhs_rhs<dir>(FWD(lhs), FWD(rhs));

        if constexpr(concepts::resizable<decltype(to)>)
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

      // Workaround for MSVC bug: https://developercommunity.visualstudio.com/t/decltype-on-autoplaceholder-parameters-deduces-wro/1594779
      template<direction dir = direction::rhs_to_lhs, concepts::associative_container lhs_t, concepts::associative_container rhs_t,
        typename converter_t = converter::identity>
      constexpr decltype(auto) operator()(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}) const
        requires (!concepts::assignable_from_converted<dir, decltype(lhs), decltype(rhs), explicit_cast<traits::lhs_t<dir, lhs_t, rhs_t>, converter_t>>)
              && requires(traits::mapped_value_forwarded_t<lhs_t> lhsElem, traits::mapped_value_forwarded_t<rhs_t> rhsElem)
                 {
                   this->template operator()<dir>(FWD(lhsElem), FWD(rhsElem), converter);
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
          [this, &lhs, &rhs, &converter](auto&& key) mutable {
            auto lhsInserter = associative_inserter(FWD(lhs), key);
            auto rhsInserter = associative_inserter(FWD(rhs), key);
            using lhs_inserter_forward_t = std_ext::like_t<decltype(lhs), decltype(lhsInserter)>;
            using rhs_inserter_forward_t = std_ext::like_t<decltype(rhs), decltype(rhsInserter)>;
            using to_mapped_t = traits::mapped_value_t<traits::lhs_t<dir, decltype(lhs), decltype(rhs)>>;
            using cast_t = explicit_cast<to_mapped_t, converter_t>;
            this->template operator()<dir, lhs_inserter_forward_t, rhs_inserter_forward_t, converter_t, cast_t>(
              std::forward<lhs_inserter_forward_t>(lhsInserter),
              std::forward<rhs_inserter_forward_t>(rhsInserter),
              converter
            );
          }
        );

        return FWD(to);
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
        if constexpr(dir == direction::rhs_to_lhs && concepts::resizable<lhs_t>)
        {
          if(lhs.size() != rhs.size())
            return false;
        }
        if constexpr(dir == direction::lhs_to_rhs && concepts::resizable<rhs_t>)
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
