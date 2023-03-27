#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <concepts>
#include <list>
#include <set>
#include <string>
#include <type_traits>
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
  // mapped_value
  {
    static_assert(std::is_same_v<const std::string, traits::mapped_value_t<std::set<std::string>>>);
    static_assert(std::is_same_v<std::string, traits::mapped_value_t<std::unordered_map<int, std::string>>>);
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
    struct type
    {
      float& operator()(float& a) { return a; }
      double operator()(double& a) { return a; }
    };

    static_assert(concepts::adaptable<int, adapter<>>);
    static_assert(concepts::adaptable<int, adapter<details::any, adapter<>>>);
    static_assert(concepts::adaptable<float, type>);
    static_assert(concepts::adaptable<double, type>);
  }

  // mappable:
  {
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::rhs_to_lhs>);
    static_assert(concepts::mappable<some_mapping, int, std::string, direction::lhs_to_rhs>);
    static_assert(!concepts::mappable<some_mapping, std::string, std::string, direction::lhs_to_rhs>);
  }

  // executable
  {
    constexpr auto lhs_to_rhs = direction::lhs_to_rhs;
    static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, int&, int&, converter::identity>);
    static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, int&, std::string&, int_string_converter>);
    static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, std::vector<int>&, std::vector<std::string>&, int_string_converter>);
    static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, std::vector<std::string>&&, std::vector<std::string>&, converter::identity>);

    constexpr auto rhs_to_lhs = direction::rhs_to_lhs;
    static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, int&, int&, converter::identity>);
    static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, int&, std::string&, int_string_converter>);
    static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, std::vector<int>&, std::vector<std::string>&, int_string_converter>);
    static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, std::vector<std::string>&, std::vector<std::string>&&, converter::identity>);
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
    static_assert(concepts::indexable<int> == false);
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
