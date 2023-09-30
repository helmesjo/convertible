#include <convertible/converters.hxx>
#include <doctest/doctest.h>
#include <libconvertible-tests/test_common.hxx>

#include <concepts>
#include <string>

TEST_CASE_TEMPLATE_DEFINE("it's invocable with types", arg_tuple_t, invocable_with_types)
{
  using converter_t = std::tuple_element_t<0, arg_tuple_t>;
  using in_t = std::tuple_element_t<1, arg_tuple_t>;
  using out_t = std::tuple_element_t<2, arg_tuple_t>;

  static_assert(std::invocable<converter_t, in_t>);
  static_assert(std::same_as<decltype(std::declval<converter_t>()(std::declval<in_t>())), out_t>);
}

SCENARIO("convertible: Converters")
{
  using namespace convertible;

  enum class enum_a{};
  enum class enum_b{};

  GIVEN("explicit_cast")
  {
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<int, converter::identity>,
        int&,
        int&
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<int, converter::identity>,
        int&&,
        int&&
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<std::string, converter::identity>,
        std::string&,
        std::string&
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<std::string, converter::identity>,
        const std::string&,
        const std::string&
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<std::string, int_string_converter>,
        int&,
        std::string
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<std::string, int_string_converter>,
        const int&,
        std::string
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<int, int_string_converter>,
        std::string&,
        int
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<proxy<std::string>, int_string_converter>,
        int&,
        std::string
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        converter::explicit_cast<proxy<const std::string>, int_string_converter>,
        const int&,
        std::string
      >
    );
  }
}
