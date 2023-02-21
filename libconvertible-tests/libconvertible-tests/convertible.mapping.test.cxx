#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <cstring>
#include <string>

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

  auto verify_empty = [](auto&& rhs){
    return rhs == std::remove_cvref_t<decltype(rhs)>{};
  };

  template<typename verify_t = decltype(verify_empty)>
  void MAPS_CORRECTLY(auto&& lhs, auto&& rhs, const auto& map, verify_t verifyMoved = verify_empty)
  {
    using namespace convertible;
    using map_t = std::remove_cvref_t<decltype(map)>;
    using lhs_t = std::remove_cvref_t<decltype(lhs)>;
    using rhs_t = std::remove_cvref_t<decltype(rhs)>;

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
    constexpr auto map = mapping(object(), object());
    (void)map;
  }
  GIVEN("a mapping between a <-> b")
  {
    using lhs_t = std::string;
    using rhs_t = std::string;

    auto map = mapping(object(), object());

    auto lhs = lhs_t{"hello"};
    auto rhs = rhs_t{"world"};
    MAPS_CORRECTLY(lhs, rhs, map);
  }
  GIVEN("a mapping between a <- converter -> b")
  {
    using lhs_t = int;
    using rhs_t = std::string;

    auto map = mapping(object(), object(), int_string_converter{});

    auto lhs = lhs_t{11};
    auto rhs = rhs_t{"22"};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
      if constexpr(std::same_as<lhs_t, std::decay_t<decltype(obj)>>)
        return obj == 11;
      else
        return obj == "";
    });
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

    auto map = mapping(object(), object());

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

    auto map = mapping(object(), object());

    auto lhs = lhs_t{"1", "2", "3"};
    auto rhs = rhs_t{"3", "2", "1"};
    MAPS_CORRECTLY(lhs, rhs, map);
  }
  GIVEN("vector<string> <-> vector<int>")
  {
    using lhs_t = std::vector<std::string>;
    using rhs_t = std::vector<int>;

    auto map = mapping(object(), object(), int_string_converter{});

    auto lhs = lhs_t{"1", "2", "3"};
    auto rhs = rhs_t{1, 2, 3};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
      if constexpr(std::same_as<lhs_t, std::decay_t<decltype(obj)>>)
        return std::all_of(std::begin(obj), std::end(obj), [](const auto& elem){ return elem == ""; });
      else
        return obj[0] == 1;
    });
  }
  GIVEN("proxy <-> string")
  {
    struct proxy
    {
      explicit proxy(std::common_reference_with<std::string> auto& str) :
        str_(str)
      {}

      explicit operator std::string() const
      {
        return str_;
      }
      proxy& operator=(std::common_reference_with<std::string> auto& rhs)
      {
        str_ = rhs;
        return *this;
      }
      proxy& operator=(std::common_reference_with<std::string> auto&& rhs)
      {
        str_ = std::move(rhs);
        return *this;
      }
      bool operator==(const proxy& rhs) const
      {
        return str_ == rhs.str_;
      }
      bool operator!=(const proxy& rhs) const
      {
        return !(*this == rhs);
      }
      bool operator==(const std::common_reference_with<std::string> auto& rhs) const
      {
        return str_ == rhs;
      }
      bool operator!=(const std::common_reference_with<std::string> auto& rhs) const
      {
        return !(*this == rhs);
      }

    private:
      std::string& str_;
    };
    static_assert(!std::is_assignable_v<std::string&, proxy>);
    static_assert(concepts::castable_to<proxy&, std::string>);

    struct custom_reader
    {
      proxy operator()(std::common_reference_with<std::string> auto& obj) const
      {
        return proxy(obj);
      }
    };
    static_assert(concepts::adaptable<std::string&, custom_reader>);

    using lhs_t = std::string;
    using rhs_t = std::string;

    auto map = mapping(custom<std::string>(custom_reader{}), object());

    auto lhs = lhs_t{"hello"};
    auto rhs = rhs_t{"world"};
    MAPS_CORRECTLY(lhs, rhs, map, [](const auto&){
      return true;
    });
  }
}
