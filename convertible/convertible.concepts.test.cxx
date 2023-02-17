#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <array>
#include <concepts>
#include <string>
#include <type_traits>

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
    template<typename operator_t, DIR_DECL(convertible::direction) dir>
    void exec(int, int){}
  };
}

SCENARIO("convertible: Traits")
{
  using namespace convertible;

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

    //detect<traits::unique_derived_types_t<base, derived>> ads;
    static_assert(std::is_same_v<std::tuple<derived_a>, traits::unique_derived_types_t<base, derived_a>>);
    static_assert(std::is_same_v<std::tuple<derived_b>, traits::unique_derived_types_t<base, derived_a, derived_b>>);
    static_assert(std::is_same_v<std::tuple<derived_b, derived_c>, traits::unique_derived_types_t<base, derived_a, derived_b, derived_c>>);
  }
}

SCENARIO("convertible: Concepts")
{
  using namespace convertible;

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

  // indexable
  {
    static_assert(concepts::indexable<std::array<int, 1>>);
    static_assert(concepts::indexable<int*>);
    static_assert(concepts::indexable<int> == false);
  }

  // dereferencable
  {
    static_assert(concepts::dereferencable<int*>);
    static_assert(concepts::dereferencable<int> == false);
  }

  // adaptable:
  {
    struct type
    {
      float& operator()(float& a) { return a; }
      double operator()(double& a) { return a; }
    };

    static_assert(concepts::adaptable<int, object<>>);
    static_assert(concepts::adaptable<int, object<object<>>>);
    static_assert(concepts::adaptable<float, type>);
    static_assert(concepts::adaptable<double, type>);
  }

  // mappable:
  {
    struct dummy_op{};
    static_assert(concepts::mappable<some_mapping, dummy_op, direction::lhs_to_rhs, int, int>);
    struct dummy{};
    static_assert(concepts::mappable<some_mapping, dummy_op, direction::lhs_to_rhs, int, dummy> == false);
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
}
