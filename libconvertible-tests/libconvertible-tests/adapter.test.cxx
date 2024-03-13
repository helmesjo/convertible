#include <convertible/convertible.hxx>
#include <libconvertible-tests/test_common.hxx>
#include <doctest/doctest.h>

#include <array>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace
{
  struct invalid_type
  {
    invalid_type() = delete;
  };

  struct type_m{};
  struct type_implicitly_convertible_to_m
  {
    // deliberately return references to verify
    // adapter does not create a copy when converting
    operator const type_m&() const &   { return m; }
    operator type_m&() &               { return m; }
    operator const type_m&&() const && { return std::move(m); }
    operator type_m&&() &&             { return std::move(m); }
    type_m m;
  };
  static_assert(convertible::concepts::castable_to<type_implicitly_convertible_to_m&, type_m&>);

  struct reader_invocable_with_type_m
  {
    auto operator()(auto&& arg) const -> decltype(auto)
      requires std::same_as<type_m, std::remove_cvref_t<decltype(arg)>>
    {
      return std::forward<decltype(arg)>(arg);
    }
  };
}

SCENARIO("convertible: Adapters")
{
  using namespace convertible;

  // implicit conversion
  GIVEN("an adapter with a reader accepting type_m")
  {
    auto adapter = convertible::adapter(type_m{}, reader_invocable_with_type_m{});
    WHEN("adapter is invoked with type_implicitly_convertible_to_m")
    {
      THEN("adapter implicitly converts the argument")
      {
        auto input = type_implicitly_convertible_to_m{};
        decltype(auto) returned = adapter(input);
        REQUIRE(&returned == &input.m);
      }
      THEN("const input& returns const type_m&")
      {
        const auto input = type_implicitly_convertible_to_m{};
        decltype(auto) returned = adapter(input);
        static_assert(std::is_same_v<decltype(returned), const type_m&>);
      }
      THEN("const input&& returns const type_m&&")
      {
        const auto input = type_implicitly_convertible_to_m{};
        decltype(auto) returned = adapter(std::move(input));
        static_assert(std::is_same_v<decltype(returned), const type_m&&>);
      }
      THEN("input& returns type_m&")
      {
        auto input = type_implicitly_convertible_to_m{};
        decltype(auto) returned = adapter(input);
        static_assert(std::is_same_v<decltype(returned), type_m&>);
      }
      THEN("input&& returns type_m&&")
      {
        auto input = type_implicitly_convertible_to_m{};
        decltype(auto) returned = adapter(std::move(input));
        static_assert(std::is_same_v<decltype(returned), type_m&&>);
      }
    }
  }

  GIVEN("identity adapter")
  {
    std::string adaptee = "hello";
    auto adapter = convertible::identity(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    // identity-reader works for similar types that share a common reference type
    static_assert(concepts::adaptable<decltype(""), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it works with const adaptee")
    {
      const auto const_adaptee = adaptee;
      REQUIRE(adapter(const_adaptee) == adaptee);
    }
    THEN("it's constexpr constructible")
    {
      struct type_x{};
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = convertible::identity(tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns value")
    {
      adapter(adaptee) = "world";
      REQUIRE(adaptee == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee);
    }
    THEN("implicit conversion 'moves from' r-value reference")
    {
      std::string assigned = adapter(std::move(adaptee));
      REQUIRE(assigned == "hello");
      REQUIRE(adaptee == "");
    }
    THEN("assign 'moves from' r-value reference")
    {
      std::string str2 = "hello";
      adapter(adaptee) = std::move(str2);
      REQUIRE(str2 == "");
    }
    THEN("equality operator works")
    {
      adaptee = "world";
      REQUIRE(adapter(adaptee) == "world");
      REQUIRE(adapter(adaptee) != "hello");
      REQUIRE("world" == adapter(adaptee));
      REQUIRE("hello" != adapter(adaptee));
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
  }
  GIVEN("member adapter (field)")
  {
    struct type
    {
      auto operator==(const type&) const -> bool = default;
      std::string str;
    } adaptee;
    adaptee.str = "hello";

    auto adapter = member(&type::str, adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it works with const adaptee")
    {
      const auto const_adaptee = adaptee;
      REQUIRE(adapter(const_adaptee) == adaptee.str);
    }
    THEN("it's constexpr constructible")
    {
      struct type_x{ int x; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = member(&type_x::x, tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(adaptee.str == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee.str);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string moved_to = adapter(std::move(adaptee));
      REQUIRE(moved_to == "hello");
      REQUIRE(adaptee.str == "");

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
    THEN("it works with derived types")
    {
      struct type_derived: public type {} adapteeDerived;
      adapteeDerived.str = "world";
      REQUIRE(adapter(adapteeDerived) == "world");
    }
  }
  GIVEN("member adapter (function)")
  {
    struct type
    {
      auto operator==(const type&) const -> bool = default;
      auto str() -> std::string&{ return str_; }
    private:
      std::string str_;
    } adaptee;
    adaptee.str() = "hello";

    auto adapter = member(&type::str, adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    // can't overload with 'const str()' because of ambiguity
    // THEN("it works with const adaptee")
    // {
    //   const auto constAdaptee = adaptee;
    //   REQUIRE(adapter(constAdaptee) == adaptee.str());
    // }
    THEN("it's constexpr constructible")
    {
      struct type_x{ [[nodiscard]] auto x() const -> int { return 0; }; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = member(&type_x::x, tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(adaptee.str() == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee.str());
    }
    THEN("it 'moves from' r-value reference")
    {
      WHEN("member function has an r-value overload")
      {
        struct type_rvalue_overload
        {
          auto str() && -> std::string&& { return std::move(str_mbr); }
          std::string str_mbr;
        } adaptee;
        adaptee.str_mbr = "hello";
        auto adapterRvalueFunc = member(&type_rvalue_overload::str);
        const std::string moved_to = adapterRvalueFunc(std::move(adaptee));
        REQUIRE(moved_to == "hello");
        REQUIRE(adaptee.str_mbr == "");
      }
      WHEN("member function has l-value overload it does NOT move")
      {
        const std::string moved_to = adapter(std::move(adaptee));
        REQUIRE(moved_to == "hello");
        REQUIRE(adaptee.str() == "hello");
      }
      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
    THEN("it works with derived types")
    {
      struct type_derived: public type {} adapteeDerived;
      adapteeDerived.str() = "world";
      REQUIRE(adapter(adapteeDerived) == "world");
    }
  }
  GIVEN("index adapter (integer)")
  {
    auto adaptee = std::array{std::string("hello")};
    auto adapter = index<0>(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it works with const adaptee")
    {
      const auto const_adaptee = adaptee;
      REQUIRE(adapter(const_adaptee) == adaptee[0]);
    }
    THEN("it's constexpr constructible")
    {
      static constexpr auto tmp = std::array{""};
      static constexpr auto constexpr_adapter = index<0>(tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(adaptee[0] == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee[0]);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string moved_to = adapter(std::move(adaptee));
      REQUIRE(moved_to == "hello");
      REQUIRE(adaptee[0] == "");

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
  }
  GIVEN("index adapter (string)")
  {
    auto adaptee = std::unordered_map<std::string, std::string>{{std::string("key"), std::string("hello")}};
    auto adapter = index<"key">(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    // unordered_map does not have a const index operator
    // THEN("it works with const adaptee")
    // {
    //   const auto constAdaptee = adaptee;
    //   REQUIRE(adapter(constAdaptee) == adaptee["key"]);
    // }
    // unordered_map is not constexpr constructible
    // THEN("it's constexpr constructible")
    // {
    //   static constexpr auto tmp = std::unordered_map<const char*, const char*>{{"", ""}};
    //   static constexpr auto constexprAdapter = index<"key">(tmp);
    //   (void)constexprAdapter;
    // }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(adaptee["key"] == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee["key"]);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string moved_to = adapter(std::move(adaptee));
      REQUIRE(moved_to == "hello");
      REQUIRE(adaptee["key"] == "");

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
  }
  GIVEN("dereference adapter")
  {
    auto str = std::string("hello");
    auto adaptee = &str;
    auto adapter = deref(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it works with const adaptee")
    {
      const auto const_adaptee = adaptee;
      REQUIRE(adapter(const_adaptee) == *adaptee);
    }
    THEN("it's constexpr constructible")
    {
      struct type_x
      {
        auto operator*() const -> int
        {
          return *x;
        }
        int* x;
      };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = deref(tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(str == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      // if adaptee is object
      {
        std::optional<std::string> tmp = "hello";
        auto adapterOpt = deref(tmp);
        std::string movedTo = adapterOpt(std::move(tmp));
        REQUIRE(movedTo == "hello");
        REQUIRE(*tmp == "");
      }
      // but not if pointer
      {
        std::string movedTo = adapter(std::move(adaptee));
        REQUIRE(movedTo == "hello");
        REQUIRE(*adaptee != "");
      }

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
  }
  GIVEN("maybe adapter")
  {
    auto adaptee = std::optional<std::string>("hello");
    auto adapter = maybe(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it is considered 'enabled' if adaptee is non-empty")
    {
      REQUIRE(adapter.enabled(adaptee));
      adaptee = std::nullopt;
      REQUIRE_FALSE(adapter.enabled(adaptee));
    }
    THEN("it works with const adaptee")
    {
      const auto const_adaptee = std::optional<std::string>("hello");
      REQUIRE(adapter(const_adaptee));
      REQUIRE(*adapter(const_adaptee) == "hello");
    }
    THEN("it's constexpr constructible")
    {
      struct type_x
      {
        auto operator*() const -> int
        {
          return *x;
        }
        int* x;
      };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = deref(tmp);
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adaptee) = "world";
      REQUIRE(*adaptee == "world");
    }
    THEN("it implicitly converts to type")
    {
      decltype(adaptee) val = adapter(adaptee);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      auto rvalueMaybe = adapter(std::move(adaptee));
      auto movedTo = *rvalueMaybe;
      REQUIRE(movedTo == "hello");
      REQUIRE(*adaptee == "");

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      REQUIRE(copy == adaptee);
    }
  }
  GIVEN("composed adapter")
  {
    struct type_a
    {
      auto operator==(const type_a&) const -> bool = default;
      operator bool() const
      {
        return !val.empty();
      }
      auto operator*() & -> std::string&
      {
        return val;
      }
      // support 'move from' deref
      auto operator*() && -> std::string&&
      {
        return std::move(val);
      }
      std::string val{};
    };
    struct type_b
    {
      auto operator==(const type_b&) const -> bool = default;
      type_a a{};
    };

    auto adaptee = type_b{type_a{"hello"}};
    auto innerAdapter = member(&type_b::a, adaptee);
    auto middleAdapter = maybe(type_a{});
    auto outerAdapter = deref();
    auto adapter = compose(innerAdapter, middleAdapter, outerAdapter);
    static_assert(concepts::adaptable<type_b, decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);
    static_assert(!concepts::adaptable<type_a, decltype(adapter)>);

    THEN("it is considered 'enabled' if adaptee is non-empty")
    {
      REQUIRE(adapter.enabled(adaptee));
      adaptee.a.val = "";
      REQUIRE_FALSE(adapter.enabled(adaptee));
    }
    THEN("it's constexpr constructible")
    {
      struct type_x{ int* x; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexpr_adapter = compose(member(&type_x::x, tmp), deref());
      (void)constexpr_adapter;
    }
    THEN("it implicitly assigns member value")
    {
      outerAdapter.reader()(middleAdapter.reader()(innerAdapter.reader()(adaptee))) = "world";
      adapter(adaptee) = "world";
      REQUIRE(adaptee.a.val == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string moved_to = adapter(std::move(adaptee));
      REQUIRE(moved_to == "hello");
      REQUIRE(adaptee.a.val == "");

      std::string fromStr = "world";
      adapter(adaptee) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adaptee) == "hello");
      REQUIRE(adapter(adaptee) != "world");
    }
    THEN("defaulted-initialized adaptee type can be created")
    {
      auto copy = adapter.defaulted_adaptee();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::adaptee_value_t>);
      INFO("adaptee.a.val: ", adaptee.a.val);
      INFO("copy.a.val: ", copy.a.val);
      REQUIRE(copy == adaptee);
    }
  }
}

SCENARIO("convertible: Adapters constexpr-ness")
{
  using namespace convertible;

  WHEN("adapter & adaptee are constexpr")
  {
    static constexpr int int_val = 1;
    static constexpr float float_val = 1.0f;

    constexpr auto adapter = convertible::adapter();

    THEN("constexpr construction")
    {
      constexpr decltype(adapter) adapter2(adapter);
      (void)adapter2;
    }
    THEN("constexpr conversion")
    {
      constexpr int val1 = adapter(int_val);
      static_assert(val1 == 1);
    }
    THEN("constexpr comparison")
    {
      static_assert(adapter(int_val) == int_val);
      static_assert(adapter(float_val) == float_val);
    }
  }
}

SCENARIO("convertible: Adapters (proxies)")
{
  using namespace convertible;

  GIVEN("string proxy adapter for custom type")
  {
    auto adapter = custom(proxy_reader{});
    std::string str;

    THEN("it implicitly assigns value")
    {
      adapter(str) = "world";
      REQUIRE(adapter(str) == std::string("world"));
    }
    THEN("it explicitly converts to type")
    {
      adapter(str) = "hello";
      auto val = static_cast<std::string>(adapter(str));
      REQUIRE(val == "hello");
    }
    THEN("equality operator works")
    {
      adapter(str) = "world";
      REQUIRE(adapter(str) == "world");
      REQUIRE(adapter(str) != "hello");
    }
  }
}
