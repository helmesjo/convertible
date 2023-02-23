#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

SCENARIO("convertible: Adapters")
{
  using namespace convertible;

  GIVEN("object adapter")
  {
    std::string adapted = "hello";
    auto adapter = object(reader::identity{}, adapted);

    THEN("it's constexpr constructible")
    {
      static constexpr decltype(adapted) tmp = {};
      static constexpr auto constexprAdapter = object(reader::identity{}, tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns value")
    {
      adapter(adapted) = "world";
      REQUIRE(adapted == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == adapted);
    }
    THEN("implicit conversion 'moves from' r-value reference")
    {
      std::string assigned = adapter(std::move(adapted));
      REQUIRE(assigned == "hello");
      REQUIRE(adapted == "");
    }
    THEN("assign 'moves from' r-value reference")
    {
      std::string str2 = "hello";
      adapter(adapted) = std::move(str2);
      REQUIRE(str2 == "");
    }
    THEN("equality operator works")
    {
      adapted = "world";
      REQUIRE(adapter(adapted) == "world");
      REQUIRE(adapter(adapted) != "hello");
      REQUIRE("world" == adapter(adapted));
      REQUIRE("hello" != adapter(adapted));
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      REQUIRE(copy == adapted);
    }
  }
  GIVEN("member adapter (field)")
  {
    struct type
    {
      auto operator<=>(const type&) const = default;
      std::string str;
    } adapted;
    adapted.str = "hello";

    auto adapter = member(&type::str, adapted);

    THEN("it's constexpr constructible")
    {
      static constexpr decltype(adapted) tmp = {};
      static constexpr auto constexprAdapter = member(&type::str, tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adapted) = "world";
      REQUIRE(adapted.str == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == adapted.str);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string movedTo = adapter(std::move(adapted));
      REQUIRE(movedTo == "hello");
      REQUIRE(adapted.str == "");

      std::string fromStr = "world";
      adapter(adapted) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adapted) == "hello");
      REQUIRE(adapter(adapted) != "world");
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      REQUIRE(copy == adapted);
    }
  }
  GIVEN("member adapter (function)")
  {
    struct type
    {
      auto operator<=>(const type&) const = default;
      std::string& str(){ return str_; }
    private:
      std::string str_;
    } adapted;
    adapted.str() = "hello";

    auto adapter = member(&type::str, adapted);

    THEN("it's constexpr constructible")
    {
      static constexpr decltype(adapted) tmp = {};
      static constexpr auto constexprAdapter = member(&type::str, tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adapted) = "world";
      REQUIRE(adapted.str() == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == adapted.str());
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string movedTo = adapter(std::move(adapted));
      REQUIRE(movedTo == "hello");
      REQUIRE(adapted.str() == "");

      std::string fromStr = "world";
      adapter(adapted) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adapted) == "hello");
      REQUIRE(adapter(adapted) != "world");
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      REQUIRE(copy == adapted);
    }
  }
  GIVEN("index adapter")
  {
    auto adapted = std::array{std::string("hello")};
    auto adapter = index<0>(adapted);

    THEN("it's constexpr constructible")
    {
      // GCC: Bug with array<string> & constexpr
      static constexpr auto tmp = std::array{""};
      static constexpr auto constexprAdapter = index<0>(tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      REQUIRE(adapted[0] == "hello");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == adapted[0]);
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string movedTo = adapter(std::move(adapted));
      REQUIRE(movedTo == "hello");
      REQUIRE(adapted[0] == "");

      std::string fromStr = "world";
      adapter(adapted) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adapted) == "hello");
      REQUIRE(adapter(adapted) != "world");
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      REQUIRE(copy == adapted);
    }
  }
  GIVEN("dereference adapter")
  {
    auto str = std::string("hello");
    auto adapted = &str;
    auto adapter = deref(adapted);

    THEN("it's constexpr constructible")
    {
      static constexpr decltype(adapted) tmp = {};
      static constexpr auto constexprAdapter = deref(tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adapted) = "world";
      REQUIRE(str == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      std::optional<std::string> tmp = "hello";
      std::string movedTo = adapter(std::move(tmp));
      REQUIRE(movedTo == "hello");
      REQUIRE(*tmp == "");

      // but not if pointer
      movedTo = adapter(std::move(adapted));
      REQUIRE(movedTo == "hello");
      REQUIRE(*adapted != "");

      std::string fromStr = "world";
      adapter(adapted) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adapted) == "hello");
      REQUIRE(adapter(adapted) != "world");
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      REQUIRE(copy == adapted);
    }
  }
  GIVEN("composed adapter")
  {
    struct type_a
    {
      auto operator<=>(const type_a&) const = default;
      std::string& operator*()
      {
        return val;
      }
      std::string val{};
    };
    struct type_b
    {
      auto operator<=>(const type_b&) const = default;
      type_a a{};
    };

    type_b adapted = {type_a{"hello"}};
    auto innerAdapter = member(&type_b::a, adapted);
    auto outerAdapter = deref();
    auto adapter = compose(outerAdapter, innerAdapter);

    THEN("it's constexpr constructible")
    {
      static constexpr decltype(adapted) tmp = {};
      static constexpr auto constexprAdapter = compose(deref(), member(&type_b::a, tmp));
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(adapted) = "world";
      REQUIRE(adapted.a.val == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(adapted);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      const std::string movedTo = adapter(std::move(adapted));
      REQUIRE(movedTo == "hello");
      REQUIRE(adapted.a.val == "");

      std::string fromStr = "world";
      adapter(adapted) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      REQUIRE(adapter(adapted) == "hello");
      REQUIRE(adapter(adapted) != "world");
    }
    THEN("defaulted-initialized adapted type can be created")
    {
      auto copy = adapter.defaulted_adapted();
      static_assert(std::same_as<decltype(copy), typename decltype(adapter)::object_value_t>);
      INFO("adapted.a.val: ", adapted.a.val);
      INFO("copy.a.val: ", copy.a.val);
      REQUIRE(copy == adapted);
    }
  }
}

SCENARIO("convertible: Adapter composition")
{
  using namespace convertible;

  struct type
  {
    std::string* val = nullptr;
  };

  std::string str;
  type obj{ &str };

  GIVEN("composed adapter")
  {
    auto adapter = deref(member(&type::val));

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = deref(object(member(&type::val)));
      constexpr auto constexprAdapter2 = member(&std::string::c_str, member(&type::val, deref()));
      (void)constexprAdapter;
      (void)constexprAdapter2;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(obj) = "hello";
      REQUIRE(str == "hello");
    }
    THEN("it implicitly converts to type")
    {
      adapter(obj) = "hello";
      std::string val = adapter(obj);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      str = "hello";
      type obj{ &str };

      const std::string movedTo = adapter(std::move(obj));
      REQUIRE(movedTo == "hello");
      REQUIRE(str == "");

      std::string fromStr = "hello";
      adapter(obj) = std::move(fromStr);
      REQUIRE(fromStr == "");
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

    constexpr auto adapter = custom(custom_reader{});
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
