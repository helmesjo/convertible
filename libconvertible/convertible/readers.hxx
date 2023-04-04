#pragma once

#include <convertible/concepts.hxx>
#include <convertible/common.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible::reader
{
  template<typename adaptee_t = details::any>
  struct identity
  {
    constexpr decltype(auto) operator()(auto&& obj) const
      requires std::is_same_v<adaptee_t, details::any>
    {
      return FWD(obj);
    }

    constexpr decltype(auto) operator()(auto&& obj) const
      requires std::common_reference_with<adaptee_t, decltype(obj)>
    {
      return FWD(obj);
    }
  };

  template<std_ext::member_ptr member_ptr_t>
  struct member
  {
    using class_t = traits::member_class_t<member_ptr_t>;

    constexpr member(member_ptr_t ptr):
      ptr_(std::move(ptr))
    {}

    template<typename obj_t>
      requires std::derived_from<std::remove_reference_t<obj_t>, class_t>
    constexpr decltype(auto) operator()(obj_t&& obj) const
    {
      // MSVC bug: 'FWD(obj).*ptr' causes 'fatal error C1001: Internal compiler error'
      //            when 'obj' is r-value reference (in combination with above 'requires' etc.)
      if constexpr(std::is_member_object_pointer_v<member_ptr_t>)
        return std::forward<std_ext::like_t<decltype(obj), decltype(obj.*ptr_)>>(obj.*ptr_);
      if constexpr(std::is_member_function_pointer_v<member_ptr_t>)
        return (FWD(obj).*ptr_)();
    }

    member_ptr_t ptr_;
  };

  template<details::const_value i>
  struct index
  {
    constexpr decltype(auto) operator()(std_ext::indexable<decltype(i)> auto&& obj) const
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
    constexpr decltype(auto) operator()(std_ext::dereferencable auto&& obj) const
    {
      return *FWD(obj);
    }
  };

  struct maybe
  {

    template<std_ext::dereferencable object_t>
    struct maybe_wrapper
    {
      using value_t = std::remove_reference_t<decltype(*std::declval<object_t>())>;
      using object_ref_t = std::remove_reference_t<object_t>&;

      maybe_wrapper(object_ref_t obj)
      : obj_(obj)
      {}

      auto& object_or_defaulted()
      {
        if(obj_)
          return obj_;
        else
          return defaulted_;
      }
      const auto& object_or_defaulted() const
      {
        return const_cast<maybe_wrapper&>(*this).object_or_defaulted();
      }

      decltype(auto) operator*()
      {
        if(obj_)
        {
          return *std::forward<object_t>(obj_);
        }
        else
        {
          defaulted_ = value_t{};
          return *std::forward<object_t>(defaulted_);
        }
      }

      constexpr operator object_t&() { return object_or_defaulted(); }
      constexpr operator const object_t&() const { return object_or_defaulted(); }
      constexpr operator bool() const { return true; }
      decltype(auto) operator<=>(auto&& rhs) const { return obj_ <=> FWD(rhs); }
      decltype(auto) operator==(auto&& rhs) const { return obj_ == FWD(rhs); }
      decltype(auto) operator!=(auto&& rhs) const { return obj_ != FWD(rhs); }
      decltype(auto) operator=(auto&& rhs) { return obj_ = FWD(rhs); }

      object_ref_t obj_;
      value_t defaultedValue_{};
      std::remove_reference_t<object_t> defaulted_{ value_t{} };
    };
    template<std_ext::dereferencable obj_t>
    maybe_wrapper(obj_t&& obj) -> maybe_wrapper<decltype(obj)>;

    template<std_ext::dereferencable object_t,
                             typename value_t = std::remove_cvref_t<decltype(*std::declval<object_t>())>>
    constexpr decltype(auto) operator()(object_t&& obj) const
      requires std::constructible_from<bool, object_t> &&
               std::is_assignable_v<object_t, value_t>
    {
      return maybe_wrapper<decltype(obj)>(obj);
    }
  };

  template<std::size_t first, std::size_t last>
    requires (first <= last)
  struct binary
  {
    template<std_ext::fixed_size_container cont_t>
    static constexpr std::size_t range_fixed_size_bytes =
      std::size(std::remove_reference_t<cont_t>{}) * sizeof(traits::range_value_t<cont_t>);

    template<std_ext::sequence_container cont_t>
    static constexpr std::size_t range_size_bytes(cont_t&& cont)
    {
      return std::size(FWD(cont)) * sizeof(traits::range_value_t<cont_t>);
    }

    template<std_ext::sequence_container storage_t, std::size_t _first, std::size_t _last>
      requires (_first <= _last)
            && std_ext::trivially_copyable<traits::range_value_t<std::remove_reference_t<storage_t>>>
    struct binary_proxy
    {
      using storage_value_t = traits::range_value_t<std::remove_reference_t<storage_t>>;
      using byte_t = std::remove_reference_t<std_ext::like_t<storage_value_t, std::byte>>;
      using const_byte_t = std::add_const_t<byte_t>;

      static constexpr bool dynamic = (!std_ext::fixed_size_container<storage_t>);
      static constexpr std::size_t first_byte = _first;
      static constexpr std::size_t last_byte = _last;
      static constexpr std::size_t byte_count = last_byte - first_byte + 1;
      static constexpr std::size_t bytes_required = first_byte + byte_count;

      // trivial type
      constexpr binary_proxy(std_ext::trivially_copyable auto& obj)
        requires (!std_ext::range<decltype(obj)>) && std::is_same_v<std::remove_cvref_t<storage_t>, std::span<byte_t>>
              && (sizeof(obj) >= byte_count)
      : bytes_(std::span{reinterpret_cast<byte_t*>(std::addressof(obj)), byte_count})
      {
      }

      // fixed size container
      constexpr binary_proxy(std_ext::fixed_size_container auto&& bytes)
        requires (range_fixed_size_bytes<decltype(bytes)> >= byte_count)
      : bytes_(bytes)
      {
      }

      // dynamic container
      constexpr binary_proxy(std_ext::sequence_container auto&& bytes)
        requires (!std_ext::fixed_size_container<decltype(bytes)>)
      : bytes_(bytes)
      {
        if constexpr(std_ext::resizable<storage_t>)
        {
          if(range_size_bytes(bytes_) < bytes_required)
          {
            const std::size_t newSize = std::max(std::size_t{1}, bytes_required / sizeof(storage_value_t));
            bytes_.resize(newSize);
          }
        }
        if(range_size_bytes(bytes_) < bytes_required)
        {
          throw std::overflow_error(
            "[binary_proxy::ctor] bytes(storage) " + std::to_string(range_size_bytes(bytes_))
            + " < bytes(proxy) " + std::to_string(size())
          );
        }
      }

      // make `trivially_copyable<binary_proxy> == false`
      ~binary_proxy(){}

      constexpr byte_t* data() noexcept
        requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
      {
        return reinterpret_cast<byte_t*>(std::data(bytes_)) + first_byte;
      }

      constexpr const_byte_t* data() const noexcept
      {
        return reinterpret_cast<const_byte_t*>(std::data(bytes_)) + first_byte;
      }

      constexpr std::size_t size() const noexcept
      {
        return byte_count;
      }

      constexpr decltype(auto) storage()
        requires (!std::is_const_v<std::remove_reference_t<storage_t>>)
      {
        return bytes_;
      }

      constexpr decltype(auto) storage() const
        requires std::is_const_v<std::remove_reference_t<storage_t>>
      {
        return bytes_;
      }

      // span<const_bytes_t> (implicit conversion)
      constexpr operator const std::span<const_byte_t>() const noexcept
      {
        return {data(), size()};
      }

      // trivial type (non-const lvalue reference)
      template<std_ext::trivially_copyable to_t>
        requires (!std_ext::range<to_t>)
              && (sizeof(to_t) >= byte_count)
              && (!std::is_const_v<std::remove_reference_t<storage_t>>)
      constexpr operator to_t&()
      {
        return reinterpret_cast<to_t&>(*data());
      }

      // trivial type (lvalue)
      template<std_ext::trivially_copyable to_t>
        requires (!std_ext::range<to_t>)
              && (sizeof(to_t) >= byte_count)
      constexpr explicit operator to_t() const
      {
        return reinterpret_cast<const to_t&>(*data());
      }

      // trivial type
      template<std_ext::trivially_copyable obj_t>
      constexpr binary_proxy& operator=(const obj_t& obj)
        requires (!std_ext::fixed_size_container<obj_t>) &&
                 (sizeof(obj) <= byte_count)
      {
        *this = {reinterpret_cast<const std::byte*>(std::addressof(obj)), sizeof(obj)};
        return *this;
      }

      // fixed size container
      template<std_ext::fixed_size_container cont_t>
      constexpr binary_proxy& operator=(const cont_t& range)
        requires std_ext::trivially_copyable<traits::range_value_t<cont_t>> &&
                 (range_fixed_size_bytes<cont_t> <= byte_count)
      {
        *this = {reinterpret_cast<const std::byte*>(std::data(range)), std::size(range)};
        return *this;
      }

      // trivial type
      template<std_ext::trivially_copyable obj_t>
      constexpr bool operator==(const obj_t& obj) const
        requires (!std_ext::fixed_size_container<obj_t>) &&
                 (sizeof(obj) <= byte_count)
      {
        return *this == std::span{reinterpret_cast<const std::byte*>(std::addressof(obj)), sizeof(obj)};
      }

      // fixed size container
      template<std_ext::fixed_size_container cont_t>
      constexpr bool operator==(const cont_t& range) const
        requires std_ext::trivially_copyable<traits::range_value_t<cont_t>> &&
                 (range_fixed_size_bytes<cont_t> <= byte_count)
      {
        return *this == std::span{reinterpret_cast<const std::byte*>(std::data(range)), std::size(range)};
      }

    private:
      constexpr binary_proxy& operator=(std::span<const std::byte> src)
      {
        if constexpr(std_ext::resizable<storage_t>)
        {
          if(src.size() > range_size_bytes(bytes_))
          {
            throw std::overflow_error(
              "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_))
              + " > bytes(storage) " + std::to_string(range_size_bytes(bytes_))
              + " - storage resized after binary_proxy construction"
            );
          }
        }
        std::memcpy(data(), std::data(src), std::size(src));
        return *this;
      }

      constexpr bool operator==(std::span<const std::byte> src) const
      {
        if constexpr(std_ext::resizable<storage_t>)
        {
          if(src.size() > range_size_bytes(bytes_))
          {
            throw std::overflow_error(
              "[binary_proxy::operator=] bytes(src) " + std::to_string(range_size_bytes(bytes_))
              + " > bytes(storage) " + std::to_string(range_size_bytes(bytes_))
              + " - storage resized after binary_proxy construction"
            );
          }
        }
        return std::memcmp(data(), std::data(src), std::size(src)) == 0;
      }

      storage_t bytes_;
    };

    // trivial type (stored as std::span)
    template<std_ext::trivially_copyable obj_t, std::size_t _first = first, std::size_t _last = last>
      requires (!std_ext::range<obj_t>)
    binary_proxy(obj_t&) ->
      binary_proxy<std::span<std::remove_reference_t<std_ext::like_t<obj_t, std::byte>>>, _first, _last>;

    // sequence container (dynamic or fixed size)
    template<std_ext::sequence_container bytes_t, std::size_t _first = first, std::size_t _last = last>
    binary_proxy(bytes_t&) -> binary_proxy<std::remove_reference_t<bytes_t>&, _first, _last>;

    constexpr decltype(auto) operator()(auto&& obj) const
      requires requires{ binary_proxy(FWD(obj)); }
    {
      return binary_proxy(FWD(obj));
    }

    // binary_proxy (by value to avoid all the different kinds of overloads)
    // creates a new proxy starting at one-past-last using the same storage
    template<typename _bytes_t, std::size_t _first, std::size_t _last, template<typename, std::size_t, std::size_t> typename _binary_proxy>
    constexpr decltype(auto) operator()(_binary_proxy<_bytes_t, _first, _last> byteProxy) const
      requires std::same_as<typename binary<_first, _last>::template binary_proxy<_bytes_t, _first, _last>,
                                                                    _binary_proxy<_bytes_t, _first, _last>>
    {
      return binary_proxy<_bytes_t, _last+1 + first, _last+1 + last>(byteProxy.storage());
    }
  };

  template<concepts::adapter... adapter_ts>
  struct composed
  {
    using adapters_t = std::tuple<adapter_ts...>;

    constexpr composed(adapter_ts... adapters):
      adapters_(std::move(adapters)...)
    {}

    static constexpr decltype(auto) compose(auto&& arg, concepts::adapter auto&& adapter)
      requires concepts::adaptable<decltype(arg), decltype(adapter)>
    {
      return FWD(adapter)(FWD(arg));
    }

    static constexpr decltype(auto) compose(auto&& arg, concepts::adapter auto&& head, concepts::adapter auto&&... tail)
      requires requires(){ FWD(head)(compose(FWD(arg), FWD(tail)...)); }
    {
      return FWD(head)(compose(FWD(arg), FWD(tail)...));
    }

    template<typename arg_t>
    constexpr decltype(auto) operator()(arg_t&& arg) const
      requires requires(){ compose(FWD(arg), std::declval<adapter_ts>()...); }
    {
      return std::apply([&arg](auto&&... adapters) -> decltype(auto) {
        return compose(std::forward<arg_t>(arg), FWD(adapters)...);
      }, adapters_);
    }

    adapters_t adapters_;
  };
}

#undef FWD
