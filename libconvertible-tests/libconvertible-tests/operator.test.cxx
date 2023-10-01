#include <convertible/operators.hxx>
#include <libconvertible-tests/test_common.hxx>
#include <doctest/doctest.h>

#include <array>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace
{
  auto verify_equal = [](const auto& lhs, const auto& rhs, const auto& converter){
    return lhs == converter(rhs);
  };

  auto verify_empty = [](auto&& rhs){
    return rhs == std::remove_cvref_t<decltype(rhs)>{};
  };
}

TEST_CASE_TEMPLATE_DEFINE("it's invocable with types", arg_tuple_t, invocable_with_types)
{
  using operator_t = std::tuple_element_t<0, arg_tuple_t>;
  using lhs_t = std::tuple_element_t<1, arg_tuple_t>;
  using rhs_t = std::tuple_element_t<2, arg_tuple_t>;

  if constexpr(std::tuple_size_v<arg_tuple_t> > 3)
  {
    using converter_t = std::tuple_element_t<3, arg_tuple_t>;
    static_assert(std::invocable<operator_t, lhs_t&, rhs_t&, converter_t>);
    static_assert(std::invocable<operator_t, lhs_t&, rhs_t&&, converter_t>);
    static_assert(std::invocable<operator_t, lhs_t&, const rhs_t&, converter_t>);
  }
  else
  {
    static_assert(std::invocable<operator_t, lhs_t&, rhs_t&>);
    static_assert(std::invocable<operator_t, lhs_t&, rhs_t&&>);
    static_assert(std::invocable<operator_t, lhs_t&, const rhs_t&>);
  }
}

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity, typename verify_t = decltype(verify_equal)>
void COPY_ASSIGNS_CORRECTLY(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}, verify_t verifyEqual = verify_equal)
{
  CAPTURE(lhs);
  CAPTURE(rhs);
  CAPTURE(converter);

  auto op = convertible::operators::assign{};

  AND_WHEN("lhs = rhs")
  {
    THEN("lhs == rhs")
    {
      op.template operator()<convertible::direction::rhs_to_lhs>(lhs, rhs, converter);
      REQUIRE(verifyEqual(lhs, rhs, converter));
    }
    THEN("lhs& is returned")
    {
      auto& res = op.template operator()<convertible::direction::rhs_to_lhs>(lhs, rhs, converter);
      static_assert(std::is_lvalue_reference_v<decltype(res)>);
      REQUIRE(&res == &lhs);
    }
  }
}

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity, typename verify_t = decltype(verify_empty)>
void MOVE_ASSIGNS_CORRECTLY(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}, verify_t verifyMoved = verify_empty)
{
  CAPTURE(lhs);
  CAPTURE(rhs);
  CAPTURE(converter);

  auto op = convertible::operators::assign{};

  // Not implemented by MSVC
  // static_assert(std::movable<std::remove_reference_t<rhs_t>>, "rhs must be a movable type");

  AND_WHEN("passed lhs & rhs (r-value)")
  {
    op.template operator()<convertible::direction::rhs_to_lhs>(lhs, std::move(rhs), converter);

    THEN("b is moved from")
    {
      REQUIRE(verifyMoved(rhs));
    }
  }
}

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity>
void EQUALITY_COMPARES_CORRECTLY(bool expectedResult, lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
{
  CAPTURE(lhs);
  CAPTURE(rhs);
  CAPTURE(converter);

  auto op = convertible::operators::equal{};

  if(expectedResult)
  {
    THEN("lhs == rhs returns true")
    {
      REQUIRE(op(lhs, rhs, converter));
    }
  }
  else
  {
    THEN("lhs != rhs returns false")
    {
      REQUIRE_FALSE(op(lhs, rhs, converter));
    }
  }
}

SCENARIO("convertible: Operators")
{
  using namespace convertible;

  struct int_string_converter
  {
    int operator()(std::string s) const
    {
      return std::stoi(s);
    }
    std::string operator()(int i) const
    {
      return std::to_string(i);
    }
  } intStringConverter;

  enum class enum_a{};
  enum class enum_b{};

  GIVEN("assign operator")
  {
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        int,
        int
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        enum_a,
        enum_b
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        int,
        std::string,
        int_string_converter
      >
    );
    // sequence containers
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::array<int, 0>,
        std::array<int, 0>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::array<int, 0>,
        std::array<std::string, 0>,
        int_string_converter
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::vector<int>,
        std::vector<int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::vector<int>,
        std::vector<std::string>,
        int_string_converter
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::list<int>,
        std::list<int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::list<int>,
        std::list<std::string>,
        int_string_converter
      >
    );
    // sequence containers (recursive)
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::vector<std::vector<int>>,
        std::vector<std::vector<std::string>>,
        int_string_converter
      >
    );
    // associative containers
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::set<int>,
        std::set<int>
      >
    );
    // TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
    //   std::tuple<
    //     operators::assign,
    //     std::set<int>,
    //     std::set<std::string>,
    //     int_string_converter
    //   >
    // );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::unordered_map<int, int>,
        std::unordered_map<int, int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::unordered_map<int, int>,
        std::unordered_map<int, std::string>,
        int_string_converter
      >
    );
    // associative containers (recursion)
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::assign,
        std::unordered_map<int, std::unordered_map<int, int>>,
        std::unordered_map<int, std::unordered_map<int, std::string>>,
        int_string_converter
      >
    );

    WHEN("lhs int, rhs int")
    {
      auto lhs = int{1};
      auto rhs = int{2};

      COPY_ASSIGNS_CORRECTLY(lhs, rhs);
    }

    WHEN("lhs string, rhs string")
    {
      auto lhs = std::string{"1"};
      auto rhs = std::string{"2"};

      COPY_ASSIGNS_CORRECTLY(lhs, rhs);
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
    }

    WHEN("lhs int, rhs string")
    {
      auto lhs = int{1};
      auto rhs = std::string{"2"};

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter);
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter);
    }

    WHEN("lhs vector<string>, rhs vector<string>")
    {
      auto lhs = std::vector<std::string>{};
      auto rhs = std::vector<std::string>{ "2" };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs);
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
    }

    WHEN("lhs vector<int>, rhs vector<string>")
    {
      auto lhs = std::vector<int>{};
      auto rhs = std::vector<std::string>{ "2" };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs[0] == converter(rhs[0]);
      });
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
        return rhs[0] == "";
      });
    }

    WHEN("lhs array<string>, rhs array<string>")
    {
      auto lhs = std::array<std::string, 1>{};
      auto rhs = std::array<std::string, 1>{ "2" };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs);
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
    }

    WHEN("lhs array<int>, rhs array<string>")
    {
      auto lhs = std::array<int, 1>{};
      auto rhs = std::array<std::string, 1>{ "2" };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs[0] == converter(rhs[0]);
      });
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
        return rhs[0] == "";
      });
    }

    WHEN("lhs array<string>, rhs vector<string>")
    {
      auto lhs = std::array<std::string, 1>{};
      auto rhs = std::vector<std::string>{ "2" };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs[0] == converter(rhs[0]);
      });
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
        return rhs[0] == "";
      });
    }

    WHEN("lhs set<int>, rhs set<int>")
    {
      auto lhs = std::set<int>{};
      auto rhs = std::set<int>{ 2 };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
        return *lhs.find(2) == converter(*rhs.find(2));
      });
      // NOTE: can't move-from a set value (AKA key) since that would break the tree
      // MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
      //   return *rhs.find("2") == "";
      // });
    }

    // WHEN("lhs set<int>, rhs set<string>")
    // {
    //   auto lhs = std::set<int>{};
    //   auto rhs = std::set<std::string>{ {"2"} };

    //   COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
    //     return *lhs.find(2) == converter(*rhs.find("2"));
    //   });
    //   // can't move-from a set value (AKA key) since that would break the tree
    //   // MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
    //   //   return *rhs.find("2") == "";
    //   // });
    // }

    WHEN("lhs unordered_map<int, int>, rhs unordered_map<int, string>")
    {
      auto lhs = std::unordered_map<int, int>{};
      auto rhs = std::unordered_map<int, std::string>{ {0, "2"} };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs.find(0)->second == converter(rhs.find(0)->second);
      });
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
        return rhs.find(0)->second == "";
      });
    }

    WHEN("lhs unordered_map<int, unordered_map<int, int>>, rhs unordered_map<int, unordered_map<int, string>>")
    {
      auto lhs = std::unordered_map<int, std::unordered_map<int, int>>{};
      auto rhs = std::unordered_map<int, std::unordered_map<int, std::string>>{ {0, {{0, "2"}}} };

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs.find(0)->second.find(0)->second == converter(rhs.find(0)->second.find(0)->second);
      });
      MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
        return rhs.find(0)->second.find(0)->second == "";
      });
    }

    WHEN("lhs is dynamic container, rhs is dynamic container")
    {
      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::vector<std::string>{ "5" };
        auto rhs = std::vector<std::string>{ "1", "2" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs == converter(rhs);
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs.empty();
        });
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::vector<std::string>{ "5", "6" };
        auto rhs = std::vector<std::string>{ "1" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs == converter(rhs);
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs.empty();
        });
      }
    }

    WHEN("lhs is static container, rhs is dynamic container")
    {
      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::array<std::string, 1>{};
        auto rhs = std::vector<std::string>{ "1", "2" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs[0] == converter(rhs[0]);
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs[0] == "" && rhs[1] == "2";
        });
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::array<std::string, 2>{"5", "6"};
        auto rhs = std::vector<std::string>{ "1" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs[0] == converter(rhs[0]) && lhs[1] == "6";
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs[0] == "";
        });
      }
    }

    WHEN("lhs is dynamic container, rhs is static container")
    {
      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::vector<std::string>{ "1" };
        auto rhs = std::array<std::string, 2>{ "1", "2" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs.size() == 2 && lhs[0] == converter(rhs[0]) && lhs[1] == converter(rhs[1]);
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs[0] == "" && rhs[1] == "";
        });
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::vector<std::string>{ "1", "2" };
        auto rhs = std::array<std::string, 1>{ "5" };

        COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
          return lhs.size() == 1 && lhs[0] == converter(rhs[0]);
        });
        MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
          return rhs[0] == "";
        });
      }
    }
    WHEN("lhs is std::string, rhs is string proxy type")
    {
      auto lhs = std::string{ "hello" };
      std::string str = "world";
      auto rhs = proxy(str);

      COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const std::string& lhs, const proxy<std::string>& rhs, const auto& converter){
        return converter(rhs) == lhs;
      });
      // Proxy does not support "move from"
      // MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
      //   return rhs == "";
      // });
    }
  }

  GIVEN("equal operator")
  {
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        int,
        int
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        enum_a,
        enum_b
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        int,
        std::string,
        int_string_converter
      >
    );
    // sequence containers
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::array<int, 0>,
        std::array<int, 0>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::array<int, 0>,
        std::array<std::string, 0>,
        int_string_converter
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::vector<int>,
        std::vector<int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::vector<int>,
        std::vector<std::string>,
        int_string_converter
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::list<int>,
        std::list<int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::list<int>,
        std::list<std::string>,
        int_string_converter
      >
    );
    // sequence containers (recursive)
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::vector<std::vector<int>>,
        std::vector<std::vector<std::string>>,
        int_string_converter
      >
    );
    // associative containers
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::set<int>,
        std::set<int>
      >
    );
    // TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
    //   std::tuple<
    //     operators::equal,
    //     std::set<int>,
    //     std::set<std::string>,
    //     int_string_converter
    //   >
    // );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::unordered_map<int, int>,
        std::unordered_map<int, int>
      >
    );
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::unordered_map<int, int>,
        std::unordered_map<int, std::string>,
        int_string_converter
      >
    );
    // associative containers (recursion)
    TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
      std::tuple<
        operators::equal,
        std::unordered_map<int, std::unordered_map<int, int>>,
        std::unordered_map<int, std::unordered_map<int, std::string>>,
        int_string_converter
      >
    );

    WHEN("lhs int, rhs int")
    {
      auto lhs = int{1};
      auto rhs = int{1};

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
      rhs = 2;
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
    }

    WHEN("lhs string, rhs string")
    {
      auto lhs = std::string{"hello"};
      auto rhs = std::string{"hello"};

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
      rhs = "world";
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
    }

    WHEN("lhs int, rhs string")
    {
      auto lhs = int{1};
      auto rhs = std::string{"1"};

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      rhs = std::string{ "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs vector<string>, rhs vector<string>")
    {
      auto lhs = std::vector<std::string>{ "hello" };
      auto rhs = std::vector<std::string>{ "hello" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
      rhs = { "world" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
    }

    WHEN("lhs vector<int>, rhs vector<string>")
    {
      auto lhs = std::vector<int>{ 1 };
      auto rhs = std::vector<std::string>{ "1" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      rhs = { "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs array<string>, rhs array<string>")
    {
      auto lhs = std::array<std::string, 1>{ "1" };
      auto rhs = std::array<std::string, 1>{ "1" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
      rhs = { "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
    }

    WHEN("lhs array<int>, rhs array<string>")
    {
      auto lhs = std::array<int, 1>{ 1 };
      auto rhs = std::array<std::string, 1>{ "1" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      rhs = { "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs array<string>, rhs vector<string>")
    {
      auto lhs = std::array<std::string, 1>{ "1" };
      auto rhs = std::vector<std::string>{ "1" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
      rhs = { "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
    }

    WHEN("lhs set<int>, rhs set<string>")
    {
      auto lhs = std::set<int>{ 1 };
      auto rhs = std::set<std::string>{ "1" };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      rhs = { "2" };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs unordered_map<int, int>, rhs unordered_map<int, string>")
    {
      auto lhs = std::unordered_map<int, int>{ {1, 1} };
      auto rhs = std::unordered_map<int, std::string>{ { 1, "1"} };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      lhs = { {2, 1} };
      rhs = { {2, "1"} };
      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      lhs = { {1, 1} };
      rhs = { {1, "2"} };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
      lhs = { {1, 1} };
      rhs = { {2, "1"} };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs unordered_map<int, unordered_map<int, int>>, rhs unordered_map<int, unordered_map<int, string>>")
    {
      auto lhs = std::unordered_map<int, std::unordered_map<int, int>>{ {0, {{1, 1}}} };
      auto rhs = std::unordered_map<int, std::unordered_map<int, std::string>>{ {0, {{ 1, "1"}}} };

      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      lhs = { {1, {{2, 1}}} };
      rhs = { {1, {{2, "1"}}} };
      EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
      lhs = { {1, {{2, 1}}} };
      rhs = { {1, {{2, "2"}}} };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
      lhs = { {1, {{2, 1}}} };
      rhs = { {1, {{1, "2"}}} };
      EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
    }

    WHEN("lhs is dynamic container, rhs is dynamic container")
    {
      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::vector<std::string>{ "1" };
        auto rhs = std::vector<std::string>{ "1", "2" };

        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::vector<std::string>{ "5", "6" };
        auto rhs = std::vector<std::string>{ "5" };

        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }
    }

    WHEN("lhs is static container, rhs is dynamic container")
    {
      AND_WHEN("lhs size == rhs size")
      {
        auto lhs = std::array<std::string, 2>{ "1", "2" };
        auto rhs = std::vector<std::string>{ "1", "2" };

        EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
        rhs = { "2", "1" };
        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }

      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::array<std::string, 1>{ "1" };
        auto rhs = std::vector<std::string>{ "1", "2" };

        EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
        rhs = { "2", "1" };
        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::array<std::string, 2>{"5", "6"};
        auto rhs = std::vector<std::string>{ "5" };

        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }
    }

    WHEN("lhs is dynamic container, rhs is static container")
    {
      AND_WHEN("lhs size == rhs size")
      {
        auto lhs = std::vector<std::string>{ "1", "2" };
        auto rhs = std::array<std::string, 2>{ "1", "2" };

        EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
        rhs = { "2", "1" };
        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }

      AND_WHEN("lhs size < rhs size")
      {
        auto lhs = std::vector<std::string>{ "1" };
        auto rhs = std::array<std::string, 2>{ "1", "2" };

        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }

      AND_WHEN("lhs size > rhs size")
      {
        auto lhs = std::vector<std::string>{ "1", "2" };
        auto rhs = std::array<std::string, 2>{ "1" };

        EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
      }
    }
  }
}
