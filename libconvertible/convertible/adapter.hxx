#pragma once

#include <convertible/common.hxx>
#include <convertible/concepts.hxx>
#include <convertible/readers.hxx>

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
            && concepts::castable_to<obj_t, traits::like_t<decltype(obj), adaptee_t>>
            && concepts::readable<traits::like_t<decltype(obj), adaptee_t>, reader_t>
    {
      return reader_(static_cast<traits::like_t<decltype(obj), adaptee_t>>(FWD(obj)));
    }

    constexpr auto defaulted_adaptee() const
      requires (!accepts_any_adaptee)
    {
      return adaptee_;
    }

    reader_t reader_;
    const adaptee_value_t adaptee_{};
  };
}

#undef FWD
