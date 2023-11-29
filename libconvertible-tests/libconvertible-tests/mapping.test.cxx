#include <convertible/convertible.hxx>
#include <libconvertible-tests/test_common.hxx>
#include <doctest/doctest.h>

#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
  auto verify_empty = [](auto&& rhs){
    return rhs == std::remove_cvref_t<decltype(rhs)>{};
  };

  template<typename verify_t = decltype(verify_empty)>
  void MAPS_CORRECTLY(auto&& lhs, auto&& rhs, const auto& map, verify_t verifyMoved = verify_empty)
  {
    using namespace convertible;
    using map_t = std::remove_cvref_t<decltype(map)>;
    // using lhs_t = std::remove_cvref_t<decltype(lhs)>;
    // using rhs_t = std::remove_cvref_t<decltype(rhs)>;

    WHEN("assigning rhs to lhs")
    {
      map.template assign<direction::rhs_to_lhs>(lhs, rhs);

      THEN("lhs == rhs")
      {
        REQUIRE(map.template equal<direction::rhs_to_lhs>(lhs, rhs));
      }
    }
    WHEN("assigning rhs (r-value) to lhs")
    {
      map.template assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

      THEN("rhs is moved from")
      {
        REQUIRE(verifyMoved(rhs));
      }
    }
    WHEN("assigning lhs to rhs")
    {
      map.template assign<direction::lhs_to_rhs>(lhs, rhs);

      THEN("rhs == lhs")
      {
        REQUIRE(map.template equal<direction::lhs_to_rhs>(lhs, rhs));
      }
    }
    if constexpr(std::is_invocable_v<typename map_t::lhs_adapter_t, decltype(std::move(lhs))>)
    {
      WHEN("assigning lhs (r-value) to rhs")
      {
        map.template assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

        THEN("lhs is moved from")
        {
          REQUIRE(verifyMoved(lhs));
        }
      }
    }
  }
}

SCENARIO("convertible: Mapping")
{
  using namespace convertible;

  THEN("it's constexpr constructible")
  {
    constexpr auto map = mapping(adapter(), adapter());
    (void)map;
  }
  GIVEN("mapping between a <-> b")
  {
    using lhs_t = std::string;
    using rhs_t = std::string;

    auto map = mapping(adapter(), adapter());

    auto lhs = lhs_t{"hello"};
    auto rhs = rhs_t{"world"};
    MAPS_CORRECTLY(lhs, rhs, map);
  }
  GIVEN("mapping between conditional(a) <-> conditional(b)")
  {
    using lhs_t = std::optional<std::string>;
    using rhs_t = std::optional<std::string>;

    auto map = mapping(maybe(lhs_t{}), maybe(rhs_t{}));

    WHEN("lhs and rhs both contains a value")
    {
      auto lhs = lhs_t{"hello"};
      auto rhs = rhs_t{"world"};
      // "moved from" optional is not cleard (that is, it still "holds a value")
      MAPS_CORRECTLY(lhs, rhs, map, [](const auto& val){ return *val == ""; });
    }
    WHEN("lhs contains a value, rhs is empty")
    {
      auto lhs = lhs_t{"hello"};
      auto rhs = rhs_t{};
      // "moved from" optional is not cleard (that is, it still "holds a value")
      WHEN("assigning rhs to lhs")
      {
        map.template assign<direction::rhs_to_lhs>(lhs, rhs);

        THEN("lhs is not assigned")
        {
          REQUIRE(*lhs == "hello");
          REQUIRE_FALSE(map.template equal<direction::rhs_to_lhs>(lhs, rhs));
          REQUIRE_FALSE(map.template equal<direction::lhs_to_rhs>(lhs, rhs));
        }
      }
      WHEN("assigning lhs to rhs")
      {
        map.template assign<direction::lhs_to_rhs>(lhs, rhs);

        THEN("lhs == rhs")
        {
          REQUIRE(map.template equal<direction::rhs_to_lhs>(lhs, rhs));
          REQUIRE(map.template equal<direction::lhs_to_rhs>(lhs, rhs));
        }
      }
    }
    WHEN("lhs is empty, rhs contains a value, ")
    {
      auto lhs = lhs_t{};
      auto rhs = rhs_t{"world"};
      // "moved from" optional is not cleared (that is, it still "holds a value")
      WHEN("assigning rhs to lhs")
      {
        map.template assign<direction::rhs_to_lhs>(lhs, rhs);

        THEN("lhs == rhs")
        {
          REQUIRE(map.template equal<direction::rhs_to_lhs>(lhs, rhs));
          REQUIRE(map.template equal<direction::lhs_to_rhs>(lhs, rhs));
        }
      }
      WHEN("assigning lhs to rhs")
      {
        map.template assign<direction::lhs_to_rhs>(lhs, rhs);

        THEN("rhs is not assigned")
        {
          REQUIRE(*rhs == "world");
          REQUIRE_FALSE(map.template equal<direction::rhs_to_lhs>(lhs, rhs));
          REQUIRE_FALSE(map.template equal<direction::lhs_to_rhs>(lhs, rhs));
        }
      }
    }
  }
  GIVEN("mapping between a <- converter -> b")
  {
    using lhs_t = int;
    using rhs_t = std::string;

    auto map = mapping(adapter(), adapter(), int_string_converter{});

    auto lhs = lhs_t{11};
    auto rhs = rhs_t{"22"};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
      if constexpr(std::equality_comparable_with<lhs_t, decltype(obj)>)
        return obj == 11;
      else
        return obj == "";
    });
  }
  GIVEN("nested mapping 'map_ab' between \n\n\ta <-> b\n")
  {
    struct type_a
    {
      std::string val;
    };

    struct type_b
    {
      std::string val;
    };

    struct type_nested_a
    {
      type_a a;
    };

    auto map_ab = mapping(member(&type_a::val), member(&type_b::val));
    static_assert(concepts::adaptable<type_a, decltype(map_ab)>);
    static_assert(concepts::adaptable<type_b, decltype(map_ab)>);

    WHEN("mapping 'map_nb' between \n\n\tn <- map_ab -> b\n")
    {
      using lhs_t = type_nested_a;
      using rhs_t = type_b;

      auto lhs = lhs_t{ "hello" };
      auto rhs = rhs_t{ "world" };

      auto map_nb = mapping(member(&type_nested_a::a), adapter<type_b>(), map_ab);

      MAPS_CORRECTLY(lhs, rhs, map_nb, [](const auto& obj){
        if constexpr(std::common_reference_with<lhs_t, decltype(obj)>)
          return obj.a.val == "";
        else
          return obj.val == "";
      });
    }
  }
  GIVEN("mapping with known lhs & rhs types")
  {
    int lhsAdaptee = 3;
    std::string rhsAdaptee = "hello";

    auto map = mapping(adapter(lhsAdaptee, reader::identity<>{}), adapter(rhsAdaptee, reader::identity<>{}), int_string_converter{});
    using map_t = decltype(map);

    THEN("defaulted lhs type can be constructed")
    {
      auto copy = map.defaulted_lhs();
      static_assert(std::same_as<decltype(copy), typename map_t::lhs_adapter_t::adaptee_value_t>);
      REQUIRE(copy == lhsAdaptee);
    }
    THEN("defaulted rhs type can be constructed")
    {
      auto copy = map.defaulted_rhs();
      static_assert(std::same_as<decltype(copy), typename map_t::rhs_adapter_t::adaptee_value_t>);
      REQUIRE(copy == rhsAdaptee);
    }
  }
}

SCENARIO("convertible: Mapping constexpr-ness")
{
  using namespace convertible;

  WHEN("mapping is constexpr")
  {
    struct type_a{ int val = 0; };
    struct type_b{ int val = 0; };
    static constexpr type_a lhsVal;
    static constexpr type_b rhsVal;

    constexpr auto lhsAdapter = member(&type_a::val);
    constexpr auto rhsAdapter = member(&type_b::val);

    constexpr auto map = mapping(lhsAdapter, rhsAdapter);

    THEN("constexpr construction")
    {
      constexpr decltype(map) map2(map);
      (void)map2;
    }
    THEN("constexpr conversion")
    {
      constexpr type_a a = map(type_b{5});
      constexpr type_b b = map(type_a{6});

      static_assert(a.val == 5);
      static_assert(b.val == 6);
    }
    THEN("constexpr comparison")
    {
      static_assert(map.equal(lhsVal, rhsVal));
    }
  }
}

SCENARIO("convertible: Mapping as a converter")
{
  using namespace convertible;

  struct type_a
  {
    std::string val;
  };

  struct type_b
  {
    std::string val;
  };

  GIVEN("mapping between \n\n\ta <-> b\n")
  {
    auto map = mapping(member(&type_a::val), member(&type_b::val));

    WHEN("invoked with a")
    {
      type_a a = { "hello" };
      type_b b = map(a);
      THEN("it returns b")
      {
        REQUIRE(b.val == a.val);
      }
    }
    WHEN("invoked with a (r-value)")
    {
      type_a a = { "hello" };
      (void)map(std::move(a));
      THEN("a is moved from")
      {
        REQUIRE(a.val == "");
      }
    }
    WHEN("invoked with b")
    {
      type_b b = { "hello" };
      type_a a = map(b);
      THEN("it returns a")
      {
        REQUIRE(a.val == b.val);
      }
    }
    WHEN("invoked with b (r-value)")
    {
      type_b b = { "hello" };
      (void)map(std::move(b));
      THEN("b is moved from")
      {
        REQUIRE(b.val == "");
      }
    }
  }
  GIVEN("nested mapping 'map_ab' between \n\n\ta <-> b\n")
  {
    auto map_ab = mapping(member(&type_a::val), member(&type_b::val));
    static_assert(concepts::adaptable<type_a, decltype(map_ab)>);
    static_assert(concepts::adaptable<type_b, decltype(map_ab)>);

    GIVEN("mapping 'map_nb' between \n\n\tn <- map_ab -> b\n")
    {
      struct type_nested_a
      {
        type_a a;
      };

      using lhs_t = type_nested_a;
      using rhs_t = type_b;

      auto lhs = lhs_t{ "hello" };
      auto rhs = rhs_t{ "world" };

      auto map_nb = mapping(member(&type_nested_a::a), adapter<type_b>(), map_ab);

      WHEN("invoked with n")
      {
        rhs = map_nb(lhs);
        THEN("it returns b")
        {
          REQUIRE(lhs.a.val == rhs.val);
        }
      }
      WHEN("invoked with n (r-value)")
      {
        (void)map_nb(std::move(lhs));
        THEN("n is moved from")
        {
          REQUIRE(lhs.a.val == "");
        }
      }
      WHEN("invoked with b")
      {
        auto lhs = map_nb(rhs);
        THEN("it returns n")
        {
          REQUIRE(lhs.a.val == rhs.val);
        }
      }
      WHEN("invoked with b (r-value)")
      {
        (void)map_nb(std::move(rhs));
        THEN("b is moved from")
        {
          REQUIRE(rhs.val == "");
        }
      }
    }
  }
}

SCENARIO("convertible: Mapping (misc use-cases)")
{
  using namespace convertible;

  GIVEN("enum_a <-> enum_b")
  {
    enum class enum_a{ val = 0 };
    enum class enum_b{ val = 0 };

    using lhs_t = enum_a;
    using rhs_t = enum_b;

    auto map = mapping(adapter(), adapter());

    auto lhs = lhs_t::val;
    auto rhs = rhs_t::val;
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto&){
      return true;
    });
  }
  GIVEN("vector<string> <-> vector<string>")
  {
    using lhs_t = std::vector<std::string>;
    using rhs_t = std::vector<std::string>;

    auto map = mapping(adapter(), adapter());

    auto lhs = lhs_t{"1", "2", "3"};
    auto rhs = rhs_t{"3", "2", "1"};
    MAPS_CORRECTLY(lhs, rhs, map);
  }
  GIVEN("vector<string> <-> vector<int>")
  {
    using lhs_t = std::vector<std::string>;
    using rhs_t = std::vector<int>;

    auto map = mapping(adapter(), adapter(), int_string_converter{});

    auto lhs = lhs_t{"1", "2", "3"};
    auto rhs = rhs_t{1, 2, 3};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
      if constexpr(std::equality_comparable_with<lhs_t, decltype(obj)>)
        return std::all_of(std::begin(obj), std::end(obj), [](const auto& elem){ return elem == ""; });
      else
        return obj[0] == 1;
    });
  }
  GIVEN("proxy <-> string")
  {
    using lhs_t = std::string;
    using rhs_t = std::string;

    auto map = mapping(custom(proxy_reader{}), adapter());

    auto lhs = lhs_t{"hello"};
    auto rhs = rhs_t{"world"};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto&){
      return true;
    });
  }
}
