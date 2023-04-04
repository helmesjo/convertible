#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define DIR_DECL(...) int
#else
#define DIR_DECL(...) __VA_ARGS__
#endif

namespace
{
  struct int_string_converter
  {
    int operator()(std::string val) const
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

    std::string operator()(int val) const
    {
      return std::to_string(val);
    }
  };

  struct some_mapping
  {
    int operator()(std::string);
    std::string operator()(int);
  };
}

SCENARIO("convertible: Traits")
{
  using namespace convertible;

  // direction
  {
    static_assert(std::is_same_v<traits::lhs_t<direction::rhs_to_lhs, int&, double&>, int&>);
    static_assert(std::is_same_v<traits::rhs_t<direction::rhs_to_lhs, int&, double&>, double&>);

    static_assert(std::is_same_v<traits::lhs_t<direction::lhs_to_rhs, int&, double&>, double&>);
    static_assert(std::is_same_v<traits::rhs_t<direction::lhs_to_rhs, int&, double&>, int&>);
  }
}

SCENARIO("convertible: Concepts")
{
  using namespace convertible;

  // adaptable:
  {
    struct adapter_w_lvalue_arg
    {
      int& operator()(std::string);
    };
    struct adapter_w_lvalue_ref_arg
    {
      int& operator()(std::string&);
    };
    struct adapter_w_lvalue_const_ref_arg
    {
      int& operator()(const std::string&);
    };
    struct adapter_w_rvalue_ref_arg
    {
      int& operator()(std::string&&);
    };
    struct adapter_w_rvalue_const_ref_arg
    {
      int& operator()(const std::string&&);
    };

    // ref-qualifies call operators
    struct adapter_lvalue_operator
    {
      int& operator()(std::string);
    };
    struct adapter_ref_operator
    {
      int& operator()(const std::string&) &;
    };
    struct adapter_const_ref_operator
    {
      int& operator()(const std::string&) const&;
    };
    struct adapter_rvalue_ref_operator
    {
      int& operator()(const std::string&) &&;
    };
    struct adapter_const_rvalue_ref_operator
    {
      int& operator()(const std::string&) const&&;
    };

    // arg qualifies
    // lvalue
    static_assert(concepts::adaptable<std::string, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<const std::string&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<const std::string&&, adapter_w_lvalue_arg>);
    // lvalue ref
    static_assert(!concepts::adaptable<std::string, adapter_w_lvalue_ref_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string&&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<const std::string&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<const std::string&&, adapter_w_lvalue_ref_arg>);
    // lvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<const std::string&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<const std::string&&, adapter_w_lvalue_const_ref_arg>);
    // rvalue ref
    static_assert(concepts::adaptable<std::string, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string&, adapter_w_rvalue_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<const std::string&, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<const std::string&&, adapter_w_rvalue_ref_arg>);
    // rvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_w_rvalue_const_ref_arg>);
    static_assert(!concepts::adaptable<std::string&, adapter_w_rvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_rvalue_const_ref_arg>);
    static_assert(!concepts::adaptable<const std::string&, adapter_w_rvalue_const_ref_arg>);
    static_assert(concepts::adaptable<const std::string&&, adapter_w_rvalue_const_ref_arg>);

    // adapter qualifiers
    // lvalue
    static_assert(concepts::adaptable<std::string, adapter_lvalue_operator>);
    // lvalue ref
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator>);
    static_assert(concepts::adaptable<std::string, adapter_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator const&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator&&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator const &&>);
    // lvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator&&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator const &&>);
    // rvalue ref
    static_assert(concepts::adaptable<std::string, adapter_rvalue_ref_operator>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_rvalue_ref_operator&&>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator const &&>);
    // rvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator>);
    static_assert(!concepts::adaptable<std::string, adapter_const_rvalue_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_const_rvalue_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator&&>);
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator const &&>);
  }

  // mappable:
  {
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::rhs_to_lhs>);
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::lhs_to_rhs>);
    // static_assert(!concepts::mappable<some_mapping, std::string, std::string, direction::lhs_to_rhs>);
  }
}
