#pragma once

#include <convertible/common.hxx>
#include <convertible/concepts.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::reader
{
  template<typename adaptee_t = details::any>
  struct identity
  {
    constexpr decltype(auto)
    operator()(auto&& obj) const
      requires std::is_same_v<adaptee_t, details::any>
    {
      return FWD(obj);
    }

    constexpr decltype(auto)
    operator()(auto&& obj) const
      requires std::common_reference_with<adaptee_t, decltype(obj)>
    {
      return FWD(obj);
    }
  };

  template<concepts::member_ptr member_ptr_t>
  struct member
  {
    using class_t = traits::member_class_t<member_ptr_t>;

    constexpr member(member_ptr_t ptr)
      : ptr_(std::move(ptr))
    {}

    template<typename obj_t>
      requires std::derived_from<std::remove_reference_t<obj_t>, class_t>
    constexpr decltype(auto)
    operator()(obj_t&& obj) const
    {
      // MSVC bug: 'FWD(obj).*ptr' causes 'fatal error C1001: Internal compiler error'
      //            when 'obj' is r-value reference (in combination with above 'requires' etc.)
      if constexpr (std::is_member_object_pointer_v<member_ptr_t>)
      {
        return std::forward<traits::like_t<decltype(obj), decltype(obj.*ptr_)>>(obj.*ptr_);
      }
      if constexpr (std::is_member_function_pointer_v<member_ptr_t>)
      {
        return (FWD(obj).*ptr_)();
      }
    }

  private:
    member_ptr_t ptr_;
  };

  template<details::const_value i>
  struct index
  {
    constexpr decltype(auto)
    operator()(concepts::indexable<decltype(i)> auto&& obj) const
    {
      // standard containers do not have a 'move from' index operator (for legacy reasons)
      // but here we want to support it for performance reasons.
      if constexpr (std::is_rvalue_reference_v<decltype(obj)>)
      {
        return std::move(FWD(obj)[i]);
      }
      else
      {
        return FWD(obj)[i];
      }
    }
  };

  struct deref
  {
    constexpr decltype(auto)
    operator()(concepts::dereferencable auto&& obj) const
    {
      return *FWD(obj);
    }
  };

  struct maybe
  {
    constexpr decltype(auto)
    operator()(concepts::dereferencable auto&& obj) const
      requires std::constructible_from<bool, decltype(obj)>
    {
      using object_t = decltype(obj);
      using value_t  = std::remove_cvref_t<decltype(*obj)>;

      if constexpr (std::is_assignable_v<object_t, value_t>)
      {
        if (!enabled(FWD(obj)))
        {
          FWD(obj) = value_t{};
        }
      }
      return FWD(obj);
    }

    constexpr bool
    enabled(concepts::dereferencable auto&& obj) const
      requires std::constructible_from<bool, decltype(obj)>
    {
      return bool{FWD(obj)};
    }
  };

  template<std::size_t first, std::size_t last>
    requires (first <= last)
  struct binary
  {
    template<concepts::fixed_size_container cont_t>
    static constexpr std::size_t range_fixed_size_bytes =
      std::size(std::remove_reference_t<cont_t>{}) * sizeof(traits::range_value_t<cont_t>);

    template<concepts::sequence_container cont_t>
    static constexpr std::size_t
    range_size_bytes(cont_t&& cont)
    {
      return std::size(FWD(cont)) * sizeof(traits::range_value_t<cont_t>);
    }

    template<concepts::sequence_container storage_t, std::size_t _first, std::size_t _last>
      requires (_first <= _last) && concepts::trivially_copyable<
                                      traits::range_value_t<std::remove_reference_t<storage_t>>>
    struct binary_proxy
    {
      using storage_value_t = traits::range_value_t<std::remove_reference_t<storage_t>>;
      using byte_t          = std::remove_reference_t<traits::like_t<storage_value_t, std::byte>>;
      using const_byte_t    = std::add_const_t<byte_t>;

      static constexpr bool        dynamic        = (!concepts::fixed_size_container<storage_t>);
      static constexpr std::size_t first_byte     = _first;
      static constexpr std::size_t last_byte      = _last;
      static constexpr std::size_t byte_count     = last_byte - first_byte + 1;
      static constexpr std::size_t bytes_required = first_byte + byte_count;

      // trivial type
      constexpr binary_proxy(concepts::trivially_copyable auto& obj)
        requires (!concepts::range<decltype(obj)>) &&
                 std::is_same_v<std::remove_cvref_t<storage_t>, std::span<byte_t>> &&
                 (sizeof(obj) >= byte_count)
        : bytes_(std::span{reinterpret_cast<byte_t*>(std::addressof(obj)), byte_count})
      {}

      // fixed size container
      constexpr binary_proxy(concepts::fixed_size_container auto&& bytes)
        requires (range_fixed_size_bytes<decltype(bytes)> >= byte_count)
        : bytes_(bytes)
      {}

      // dynamic container
      constexpr binary_proxy(concepts::sequence_container auto&& bytes)
        requires (!concepts::fixed_size_container<decltype(bytes)>)
        : bytes_(bytes)
      {
        if constexpr (concepts::resizable_container<storage_t>)
        {
          if (range_size_bytes(bytes_) < bytes_required)
          {
            std::size_t const new_size =
              std::max(std::size_t{1}, bytes_required / sizeof(storage_value_t));
            bytes_.resize(new_size);
          }
        }
        if (range_size_bytes(bytes_) < bytes_required)
        {
          throw std::overflow_error("[binary_proxy::ctor] bytes(storage) " +
                                    std::to_string(range_size_bytes(bytes_)) + " < bytes(proxy) " +
                                    std::to_string(size()));
        }
      }

      // make `trivially_copyable<binary_proxy> == false`
      ~binary_proxy() {}

      constexpr byte_t*
      data() noexcept
        requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
      {
        return reinterpret_cast<byte_t*>(std::data(bytes_)) + first_byte;
      }

      constexpr const_byte_t*
      data() const noexcept
      {
        return reinterpret_cast<const_byte_t*>(std::data(bytes_)) + first_byte;
      }

      constexpr std::size_t
      size() const noexcept
      {
        return byte_count;
      }

      constexpr decltype(auto)
      storage()
        requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
      {
        return bytes_;
      }

      constexpr decltype(auto)
      storage() const
        requires std::is_const_v<std::remove_reference_t<storage_t>>
      {
        return bytes_;
      }

      // span<const_bytes_t> (implicit conversion)
      constexpr
      operator std::span<const_byte_t> const() const noexcept
      {
        return {data(), size()};
      }

      // trivial type (non-const lvalue reference)
      template<concepts::trivially_copyable to_t>
        requires (!concepts::range<to_t>) && (sizeof(to_t) >= byte_count) &&
                 (!std::is_const_v<std::remove_reference_t<storage_t>>)
      constexpr
      operator to_t&()
      {
        return reinterpret_cast<to_t&>(*data());
      }

      // trivial type (lvalue)
      template<concepts::trivially_copyable to_t>
        requires (!concepts::range<to_t>) && (sizeof(to_t) >= byte_count)
      constexpr explicit
      operator to_t() const
      {
        return reinterpret_cast<to_t const&>(*data());
      }

      // trivial type
      template<concepts::trivially_copyable obj_t>
      constexpr binary_proxy&
      operator=(obj_t const& obj)
        requires (!concepts::fixed_size_container<obj_t>) && (sizeof(obj) <= byte_count)
      {
        *this = {reinterpret_cast<std::byte const*>(std::addressof(obj)), sizeof(obj)};
        return *this;
      }

      // fixed size container
      template<concepts::fixed_size_container cont_t>
      constexpr binary_proxy&
      operator=(cont_t const& range)
        requires concepts::trivially_copyable<traits::range_value_t<cont_t>> &&
                 (range_fixed_size_bytes<cont_t> <= byte_count)
      {
        *this = {reinterpret_cast<std::byte const*>(std::data(range)), std::size(range)};
        return *this;
      }

      // trivial type
      template<concepts::trivially_copyable obj_t>
      constexpr bool
      operator==(obj_t const& obj) const
        requires (!concepts::fixed_size_container<obj_t>) && (sizeof(obj) <= byte_count)
      {
        return *this ==
               std::span{reinterpret_cast<std::byte const*>(std::addressof(obj)), sizeof(obj)};
      }

      // fixed size container
      template<concepts::fixed_size_container cont_t>
      constexpr bool
      operator==(cont_t const& range) const
        requires concepts::trivially_copyable<traits::range_value_t<cont_t>> &&
                 (range_fixed_size_bytes<cont_t> <= byte_count)
      {
        return *this ==
               std::span{reinterpret_cast<std::byte const*>(std::data(range)), std::size(range)};
      }

    private:
      constexpr binary_proxy&
      operator=(std::span<std::byte const> src)
      {
        if constexpr (concepts::resizable_container<storage_t>)
        {
          if (src.size() > range_size_bytes(bytes_))
          {
            throw std::overflow_error(
              "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_)) +
              " > bytes(storage) " + std::to_string(range_size_bytes(bytes_)) +
              " - storage resized after binary_proxy construction");
          }
        }
        std::memcpy(data(), std::data(src), std::size(src));
        return *this;
      }

      constexpr bool
      operator==(std::span<std::byte const> src) const
      {
        if constexpr (concepts::resizable_container<storage_t>)
        {
          if (src.size() > range_size_bytes(bytes_))
          {
            throw std::overflow_error(
              "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_)) +
              " > bytes(storage) " + std::to_string(range_size_bytes(bytes_)) +
              " - storage resized after binary_proxy construction");
          }
        }
        return std::memcmp(data(), std::data(src), std::size(src)) == 0;
      }

    private:
      storage_t bytes_;
    };

    // trivial type (stored as std::span)
    template<concepts::trivially_copyable obj_t, std::size_t _first = first,
             std::size_t _last = last>
      requires (!concepts::range<obj_t>)
    binary_proxy(obj_t&)
      -> binary_proxy<std::span<std::remove_reference_t<traits::like_t<obj_t, std::byte>>>, _first,
                      _last>;

    // sequence container (dynamic or fixed size)
    template<concepts::sequence_container bytes_t, std::size_t _first = first,
             std::size_t _last = last>
    binary_proxy(bytes_t&) -> binary_proxy<std::remove_reference_t<bytes_t>&, _first, _last>;

    constexpr decltype(auto)
    operator()(auto&& obj) const
      requires requires { binary_proxy(FWD(obj)); }
    {
      return binary_proxy(FWD(obj));
    }

    // binary_proxy (by value to avoid all the different kinds of overloads)
    // creates a new proxy starting at one-past-last using the same storage
    template<typename _bytes_t, std::size_t _first, std::size_t _last,
             template<typename, std::size_t, std::size_t> typename _binary_proxy>
    constexpr decltype(auto)
    operator()(_binary_proxy<_bytes_t, _first, _last> byteProxy) const
      requires std::same_as<
        typename binary<_first, _last>::template binary_proxy<_bytes_t, _first, _last>,
        _binary_proxy<_bytes_t, _first, _last>>
    {
      return binary_proxy<_bytes_t, _last + 1 + first, _last + 1 + last>(byteProxy.storage());
    }
  };

  template<concepts::adapter... adapter_ts>
  struct composed
  {
  private:
    using adapters_t = std::tuple<adapter_ts...>;

    template<typename arg_t, typename head_t, typename... tail_t>
    static constexpr bool
    is_composable()
    {
      if constexpr (sizeof...(tail_t) == 0)
      {
        return concepts::adaptable<arg_t, head_t>;
      }
      else if constexpr (is_composable<arg_t, head_t>())
      {
        return is_composable<traits::adapted_t<head_t, arg_t>, tail_t...>();
      }
      else
      {
        return false;
      }
    }

    static constexpr decltype(auto)
    compose(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
      requires (is_composable<decltype(arg), decltype(head), decltype(tail)...>())
    {
      if constexpr (sizeof...(tail) == 0)
      {
        return FWD(head)(FWD(arg));
      }
      else
      {
        return compose(FWD(head)(FWD(arg)), FWD(tail)...);
      }
    }

    static constexpr bool
    enabled_impl(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
      requires (is_composable<decltype(arg), decltype(head), decltype(tail)...>())
    {
      if constexpr (sizeof...(tail) == 0)
      {
        if constexpr (requires { FWD(head).enabled(FWD(arg)); })
        {
          return FWD(head).enabled(FWD(arg));
        }
        else
        {
          return true;
        }
      }
      else if (enabled_impl(FWD(arg), FWD(head)))
      {
        return enabled_impl(compose(FWD(arg), FWD(head)), FWD(tail)...);
      }
      else
      {
        return false;
      }
    }

    adapters_t adapters_;

  public:
    constexpr composed(adapter_ts... adapters)
      : adapters_(std::move(adapters)...)
    {}

    template<typename arg_t>
    constexpr decltype(auto)
    operator()(arg_t&& arg) const
      requires requires () { compose(FWD(arg), std::declval<adapter_ts>()...); }
    {
      return std::apply(
        [&arg](auto&&... adapters) -> decltype(auto)
        {
          return compose(std::forward<arg_t>(arg), FWD(adapters)...);
        },
        adapters_);
    }

    constexpr bool
    enabled(auto&& arg) const
      requires requires { enabled_impl(FWD(arg), std::declval<adapter_ts>()...); }
    {
      using arg_t = decltype(arg);
      return std::apply(
        [&arg](auto&&... adapters) -> bool
        {
          return enabled_impl(std::forward<arg_t>(arg), FWD(adapters)...);
        },
        adapters_);
    }
  };
}

#undef FWD
