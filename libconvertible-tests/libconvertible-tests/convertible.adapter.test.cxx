#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

namespace
{
  struct invalid_type
  {
    invalid_type() = delete;
  };
}

SCENARIO("convertible: Adapters")
{
  using namespace convertible;

  GIVEN("object adapter")
  {
    std::string adaptee = "hello";
    auto adapter = object(adaptee, reader::identity<>{});
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    // identity-reader works for all types, even this fake 'invalid' type
    static_assert(concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      struct type_x{};
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = object(tmp, reader::identity<>{});
      (void)constexprAdapter;
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
      bool operator==(const type&) const = default;
      std::string str;
    } adaptee;
    adaptee.str = "hello";

    auto adapter = member(&type::str, adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      struct type_x{ int x; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = member(&type_x::x, tmp);
      (void)constexprAdapter;
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
      const std::string movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
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
      bool operator==(const type&) const = default;
      std::string& str(){ return str_; }
    private:
      std::string str_;
    } adaptee;
    adaptee.str() = "hello";

    auto adapter = member(&type::str, adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      struct type_x{ int x() const { return 0; }; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = member(&type_x::x, tmp);
      (void)constexprAdapter;
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
      const std::string movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
      REQUIRE(adaptee.str() == "");

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
  GIVEN("index adapter")
  {
    auto adaptee = std::array{std::string("hello")};
    auto adapter = index<0>(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      // GCC: Bug with array<string> & constexpr
      static constexpr auto tmp = std::array{""};
      static constexpr auto constexprAdapter = index<0>(tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      REQUIRE(adaptee[0] == "hello");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adaptee);
      REQUIRE(val == adaptee[0]);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
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
  GIVEN("dereference adapter")
  {
    auto str = std::string("hello");
    auto adaptee = &str;
    auto adapter = deref(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      struct type_x
      {
        int operator*() const
        {
          return *x;
        }
        int* x;
      };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = deref(tmp);
      (void)constexprAdapter;
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
      std::optional<std::string> tmp = "hello";
      std::string movedTo = adapter(std::move(tmp));
      REQUIRE(movedTo == "hello");
      REQUIRE(*tmp == "");

      // but not if pointer
      movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
      REQUIRE(*adaptee != "");

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
      bool operator==(const type_a&) const = default;
      std::string& operator*()
      {
        return val;
      }
      std::string val{};
    };
    struct type_b
    {
      bool operator==(const type_b&) const = default;
      type_a a{};
    };

    type_b adaptee = {type_a{"hello"}};
    auto innerAdapter = member(&type_b::a, adaptee);
    auto outerAdapter = deref();
    auto adapter = compose(outerAdapter, innerAdapter);
    static_assert(concepts::adaptable<type_b, decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);
    static_assert(!concepts::adaptable<type_a, decltype(adapter)>);

    THEN("it's constexpr constructible")
    {
      struct type_x{ int* x; };
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = compose(deref(), member(&type_x::x, tmp));
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
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
      const std::string movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
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

  WHEN("object & adapter is constexpr")
  {
    static constexpr int intVal = 1;
    static constexpr float floatVal = 1.0f;

    constexpr auto adapter = object();

    THEN("constexpr construction")
    {
      constexpr decltype(adapter) adapter2(adapter);
      (void)adapter2;
    }
    THEN("constexpr conversion")
    {
      constexpr int val1 = adapter(intVal);
      static_assert(val1 == 1);
    }
    THEN("constexpr comparison")
    {
      static_assert(adapter(intVal) == intVal);
      static_assert(adapter(floatVal) == floatVal);
    }
  }
}

SCENARIO("convertible: Adapters (proxies)")
{
  using namespace convertible;

  GIVEN("string proxy adapter for custom type")
  {
    struct proxy
    {
      explicit proxy(std::string& str) :
        str_(str)
      {}

      explicit operator std::string() const
      {
        return str_;
      }
      proxy& operator=(const std::string& rhs)
      {
        str_ = rhs;
        return *this;
      }
      proxy& operator=(std::string&& rhs)
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
      bool operator==(const std::string& rhs) const
      {
        return str_ == rhs;
      }
      bool operator!=(const std::string& rhs) const
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
      proxy operator()(std::string& obj) const
      {
        return proxy(obj);
      }
    };
    static_assert(concepts::adaptable<std::string&, custom_reader>);

    auto adapter = custom(custom_reader{});
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
      REQUIRE("world" == adapter(str));
      REQUIRE("hello" != adapter(str));
    }
  }
}
