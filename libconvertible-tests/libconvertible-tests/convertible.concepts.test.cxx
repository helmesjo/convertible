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

  // member pointer
  {
    struct type
    {
      int member;
      double& fun(int, char*);
    };

    static_assert(std::is_same_v<type, convertible::traits::member_class_t<decltype(&type::member)>>);
    static_assert(std::is_same_v<int, convertible::traits::member_value_t<decltype(&type::member)>>);
    static_assert(std::is_same_v<type, convertible::traits::member_class_t<decltype(&type::fun)>>);
    static_assert(std::is_same_v<double&, convertible::traits::member_value_t<decltype(&type::fun)>>);
  }
  // range_value
  {
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>>>);
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&>>);
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&&>>);
  }
  // range_value_forwarded
  {
    static_assert(std::is_same_v<std::string&&, traits::range_value_forwarded_t<std::vector<std::string>>>);
    static_assert(std::is_same_v<std::string&, traits::range_value_forwarded_t<std::vector<std::string>&>>);
    static_assert(std::is_same_v<const std::string&&, traits::range_value_forwarded_t<const std::vector<std::string>&&>>);
  }
  // mapped_value
  {
    static_assert(std::is_same_v<std::string, traits::mapped_value_t<std::set<std::string>>>);
    static_assert(std::is_same_v<std::string, traits::mapped_value_t<std::unordered_map<int, std::string>>>);
  }
  // mapped_value_forwarded
  {
    static_assert(std::is_same_v<std::string&&, traits::mapped_value_forwarded_t<std::set<std::string>>>);
    static_assert(std::is_same_v<std::string&, traits::mapped_value_forwarded_t<std::set<std::string>&>>);
    static_assert(std::is_same_v<const std::string&, traits::mapped_value_forwarded_t<const std::set<std::string>&>>);
    static_assert(std::is_same_v<std::string&&, traits::mapped_value_forwarded_t<std::unordered_map<int, std::string>>>);
    static_assert(std::is_same_v<std::string&, traits::mapped_value_forwarded_t<std::unordered_map<int, std::string>&>>);
    static_assert(std::is_same_v<const std::string&, traits::mapped_value_forwarded_t<const std::unordered_map<int, std::string>&>>);
  }
  // unique_types
  {
    static_assert(std::is_same_v<std::tuple<int, float>, traits::unique_types_t<int, float, int, float>>);
    static_assert(std::is_same_v<std::tuple<float, int>, traits::unique_types_t<int, float, float, int>>);
  }
  // unique_derived_types
  {
    struct base {};
    struct derived_a: base {};
    struct derived_b: derived_a {};
    struct derived_c: base {};

    static_assert(std::is_same_v<std::tuple<derived_a>, traits::unique_derived_types_t<base, derived_a>>);
    static_assert(std::is_same_v<std::tuple<derived_b>, traits::unique_derived_types_t<base, derived_a, derived_b>>);
    static_assert(std::is_same_v<std::tuple<derived_b, derived_c>, traits::unique_derived_types_t<base, derived_a, derived_b, derived_c>>);
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

  // castable_to
  {
    static_assert(concepts::castable_to<int, int>);
    static_assert(concepts::castable_to<int, float>);
    static_assert(concepts::castable_to<float, int>);

    static_assert(!concepts::castable_to<std::string, int>);
    static_assert(!concepts::castable_to<int, std::string>);
  }

  // indexable
  {
    struct type
    {
      int operator[](std::size_t);
      int operator[](const char*);
    };

    static_assert(concepts::indexable<std::array<int, 1>, decltype(details::const_value(1))>);
    static_assert(concepts::indexable<type,               decltype(details::const_value("key"))>);
    static_assert(concepts::indexable<std::array<int, 1>>);
    static_assert(concepts::indexable<int*>);
    static_assert(!concepts::indexable<int>);
  }

  // dereferencable
  {
    static_assert(concepts::dereferencable<int*>);
    static_assert(concepts::dereferencable<int> == false);
  }

  // member_ptr:
  {
    struct type
    {
      int member;
      double& fun(int, char*);
    };

    static_assert(concepts::member_ptr<decltype(&type::member)>);
    static_assert(concepts::member_ptr<decltype(&type::fun)>);
    static_assert(concepts::member_ptr<type> == false);
  }

  // range
  {
    static_assert(concepts::range<int[2]>);
    static_assert(concepts::range<std::array<int, 0>>);
    static_assert(concepts::range<std::vector<int>>);
    static_assert(concepts::range<std::list<int>>);
    static_assert(concepts::range<std::set<int>>);
    static_assert(concepts::range<std::unordered_map<int, int>>);
  }
  // fixed-size container
  {
    static_assert(concepts::fixed_size_container<std::array<int, 1>>);
    static_assert(concepts::fixed_size_container<int[1]>);
    static_assert(!concepts::fixed_size_container<std::vector<int>>);
    static_assert(!concepts::fixed_size_container<std::string>);
    static_assert(!concepts::fixed_size_container<std::set<int>>);
  }
  // sequence container
  {
    static_assert(concepts::sequence_container<std::array<int, 0>>);
    static_assert(concepts::sequence_container<std::vector<int>>);
    static_assert(concepts::sequence_container<std::list<int>>);
    static_assert(!concepts::associative_container<std::array<int, 0>>);
    static_assert(!concepts::associative_container<std::vector<int>>);
    static_assert(!concepts::associative_container<std::list<int>>);
  }
  // associative container
  {
    static_assert(concepts::associative_container<std::set<int>>);
    static_assert(concepts::associative_container<std::unordered_map<int, int>>);
    static_assert(!concepts::sequence_container<std::set<int>>);
    static_assert(!concepts::sequence_container<std::unordered_map<int, int>>);
  }
  // mapping container
  {
    static_assert(concepts::mapping_container<std::unordered_map<int, int>>);
    static_assert(!concepts::mapping_container<std::set<int>>);
  }
}
