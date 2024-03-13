#pragma once

#include <convertible/concepts.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <concepts>
#include <string>
#include <type_traits>

struct int_string_converter
{
  auto operator()(std::string val) const -> int
  {
    try
    {
      return std::stoi(val);
    }
    catch(const std::exception&)
    {
      return 0;
    }
  }

  auto operator()(int val) const -> std::string
  {
    return std::to_string(val);
  }
};

template<typename T>
struct proxy
{
  static constexpr bool is_const = std::is_const_v<T>;

  explicit proxy(T& str) :
    obj_(str)
  {}
  operator const T&() const
  {
    return obj_;
  }
  operator T&()
    requires (!is_const)
  {
    return obj_;
  }
  auto operator=(const T& rhs) -> proxy&
    requires (!is_const)
  {
    obj_ = rhs;
    return *this;
  }
  auto operator=(T&& rhs) -> proxy&
    requires (!is_const)
  {
    obj_ = std::move(rhs);
    return *this;
  }
  auto operator==(const proxy& rhs) const -> bool
  {
    return obj_ == rhs.obj_;
  }
  auto operator==(const T& rhs) const -> bool
  {
    return obj_ == rhs;
  }
  friend  auto operator==(const T& lhs, const proxy& rhs) -> bool
  {
    return lhs == static_cast<decltype(lhs)>(rhs);
  }
private:
  T& obj_;
};
template<typename obj_t>
proxy(obj_t&& obj) -> proxy<std::remove_reference_t<decltype(obj)>>;

static_assert(std::common_reference_with<proxy<std::string>, std::string>);
static_assert(std::equality_comparable_with<proxy<std::string>, std::string>);
static_assert(std::assignable_from<std::string&, proxy<std::string>>);
static_assert(std::assignable_from<proxy<std::string>&, std::string>);
static_assert(convertible::concepts::castable_to<proxy<std::string>, std::string>);

struct proxy_reader
{
  auto operator()(std::string& obj) const
  {
    return proxy(obj);
  }
  auto operator()(const std::string& obj) const
  {
    return proxy(obj);
  }
};
static_assert(convertible::concepts::adaptable<std::string&, proxy_reader>);
