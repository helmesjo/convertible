#include <convertible/convertible.hxx>

#include <string>

#include <doctest/doctest.h>

#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
  #define DIR_DECL(...) int
#else
  #define DIR_DECL(...) __VA_ARGS__
#endif

namespace
{
  struct int_string_converter
  {
    auto
    operator()(std::string val) const -> int
    {
      try
      {
        return std::stoi(val);
      }
      catch (std::exception const&)
      {
        return 0;
      }
    }

    auto
    operator()(int val) const -> std::string
    {
      return std::to_string(val);
    }
  };

  struct some_mapping
  {
    auto operator()(std::string) -> int;
    auto operator()(int) -> std::string;
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
      auto operator()(std::string) -> int&;
    };

    struct adapter_w_lvalue_ref_arg
    {
      auto operator()(std::string&) -> int&;
    };

    struct adapter_w_lvalue_const_ref_arg
    {
      auto operator()(std::string const&) -> int&;
    };

    struct adapter_w_rvalue_ref_arg
    {
      auto operator()(std::string&&) -> int&;
    };

    struct adapter_w_rvalue_const_ref_arg
    {
      auto operator()(std::string const&&) -> int&;
    };

    // ref-qualifies call operators
    struct adapter_lvalue_operator
    {
      auto operator()(std::string) -> int&;
    };

    struct adapter_ref_operator
    {
      auto operator()(std::string const&) & -> int&;
    };

    struct adapter_const_ref_operator
    {
      auto operator()(std::string const&) const& -> int&;
    };

    struct adapter_rvalue_ref_operator
    {
      auto operator()(std::string const&) && -> int&;
    };

    struct adapter_const_rvalue_ref_operator
    {
      auto operator()(std::string const&) const&& -> int&;
    };

    struct type
    {
      auto operator()(int) -> int;
      void operator()(std::string);
    };

    // must return a value
    static_assert(concepts::adaptable<int, type>);
    static_assert(!concepts::adaptable<std::string, type>);

    // arg qualifies
    // lvalue
    static_assert(concepts::adaptable<std::string, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string const&, adapter_w_lvalue_arg>);
    static_assert(concepts::adaptable<std::string const&&, adapter_w_lvalue_arg>);
    // lvalue ref
    static_assert(!concepts::adaptable<std::string, adapter_w_lvalue_ref_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string&&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string const&, adapter_w_lvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string const&&, adapter_w_lvalue_ref_arg>);
    // lvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string const&, adapter_w_lvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string const&&, adapter_w_lvalue_const_ref_arg>);
    // rvalue ref
    static_assert(concepts::adaptable<std::string, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string&, adapter_w_rvalue_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string const&, adapter_w_rvalue_ref_arg>);
    static_assert(!concepts::adaptable<std::string const&&, adapter_w_rvalue_ref_arg>);
    // rvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_w_rvalue_const_ref_arg>);
    static_assert(!concepts::adaptable<std::string&, adapter_w_rvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string&&, adapter_w_rvalue_const_ref_arg>);
    static_assert(!concepts::adaptable<std::string const&, adapter_w_rvalue_const_ref_arg>);
    static_assert(concepts::adaptable<std::string const&&, adapter_w_rvalue_const_ref_arg>);

    // adapter qualifiers
    // lvalue
    static_assert(concepts::adaptable<std::string, adapter_lvalue_operator>);
    // lvalue ref
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator>);
    static_assert(concepts::adaptable<std::string, adapter_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator const&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator&&>);
    static_assert(!concepts::adaptable<std::string, adapter_ref_operator const&&>);
    // lvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator&&>);
    static_assert(concepts::adaptable<std::string, adapter_const_ref_operator const&&>);
    // rvalue ref
    static_assert(concepts::adaptable<std::string, adapter_rvalue_ref_operator>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_rvalue_ref_operator&&>);
    static_assert(!concepts::adaptable<std::string, adapter_rvalue_ref_operator const&&>);
    // rvalue const ref
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator>);
    static_assert(!concepts::adaptable<std::string, adapter_const_rvalue_ref_operator&>);
    static_assert(!concepts::adaptable<std::string, adapter_const_rvalue_ref_operator const&>);
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator&&>);
    static_assert(concepts::adaptable<std::string, adapter_const_rvalue_ref_operator const&&>);
  }

  // mappable:
  {
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::rhs_to_lhs>);
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::lhs_to_rhs>);
    // static_assert(!concepts::mappable<some_mapping, std::string, std::string,
    // direction::lhs_to_rhs>);
  }
}
