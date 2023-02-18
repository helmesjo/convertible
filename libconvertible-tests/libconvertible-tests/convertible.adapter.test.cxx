#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <cstring>
#include <string>
#include <tuple>
#include <type_traits>

SCENARIO("convertible: Adapters")
{
  using namespace convertible;

  GIVEN("object adapter")
  {
    auto adapter = object();
    std::string str = "hello";

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = object();
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns value")
    {
      adapter(str) = "world";
      REQUIRE(str == "world");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(str);
      REQUIRE(val == str);
    }
    THEN("implicit conversion 'moves from' r-value reference")
    {
      std::string assigned = adapter(std::move(str));
      REQUIRE(assigned == "hello");
      REQUIRE(str == "");
    }
    THEN("assign 'moves from' r-value reference")
    {
      std::string str2 = "hello";
      adapter(str) = std::move(str2);
      REQUIRE(str2 == "");
    }
    THEN("equality operator works")
    {
      str = "world";
      REQUIRE(adapter(str) == "world");
      REQUIRE(adapter(str) != "hello");
      REQUIRE("world" == adapter(str));
      REQUIRE("hello" != adapter(str));
    }
  }
  GIVEN("member adapter (field)")
  {
    struct type
    {
      std::string str;
    } obj;

    auto adapter = member(&type::str);

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = member(&type::str);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(obj) = "hello";
      REQUIRE(obj.str == "hello");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(obj);
      REQUIRE(val == obj.str);
    }
    THEN("it 'moves from' r-value reference")
    {
      obj.str = "world";
      const std::string movedTo = adapter(std::move(obj));
      REQUIRE(movedTo == "world");
      REQUIRE(obj.str == "");

      std::string fromStr = "hello";
      adapter(obj) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      obj.str = "world";
      REQUIRE(adapter(obj) == "world");
      REQUIRE(adapter(obj) != "hello");
    }
  }
  GIVEN("member adapter (function)")
  {
    struct type
    {
      std::string& str(){ return str_; }
    private:
      std::string str_;
    } obj;

    auto adapter = member(&type::str);

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = member(&type::str);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(obj) = "hello";
      REQUIRE(obj.str() == "hello");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(obj);
      REQUIRE(val == obj.str());
    }
    THEN("it 'moves from' r-value reference")
    {
      obj.str() = "world";
      const std::string movedTo = adapter(std::move(obj));
      REQUIRE(movedTo == "world");
      REQUIRE(obj.str() == "");

      std::string fromStr = "hello";
      adapter(obj) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      obj.str() = "world";
      REQUIRE(adapter(obj) == "world");
      REQUIRE(adapter(obj) != "hello");
    }
  }
  GIVEN("index adapter")
  {
    auto adapter = index<0>();
    std::array values = {std::string("1")};

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = index<0>();
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(values) = "hello";
      REQUIRE(values[0] == "hello");
    }
    THEN("it implicitly converts to type")
    {
      std::string val = adapter(values);
      REQUIRE(val == values[0]);
    }
    THEN("it 'moves from' r-value reference")
    {
      values[0] = "world";

      const std::string movedTo = adapter(std::move(values));
      REQUIRE(movedTo == "world");
      REQUIRE(values[0] == "");

      std::string fromStr = "hello";
      adapter(values) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      values[0] = "world";
      REQUIRE(adapter(values) == "world");
      REQUIRE(adapter(values) != "hello");
    }
  }
  GIVEN("dereference adapter")
  {   
    auto adapter = deref();

    auto str = std::string();
    auto ptr = &str;

    THEN("it's constexpr constructible")
    {
      constexpr auto constexprAdapter = deref();
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns member value")
    {
      adapter(ptr) = "hello";
      REQUIRE(str == "hello");
    }
    THEN("it implicitly converts to type")
    {
      adapter(ptr) = "hello";
      std::string val = adapter(ptr);
      REQUIRE(val == "hello");
    }
    THEN("it 'moves from' r-value reference")
    {
      str = "hello";
      const std::string movedTo = adapter(std::move(ptr));
      REQUIRE(movedTo == "hello");
      REQUIRE(str == "");

      std::string fromStr = "hello";
      adapter(ptr) = std::move(fromStr);
      REQUIRE(fromStr == "");
    }
    THEN("equality operator works")
    {
      str = "hello";
      REQUIRE(adapter(ptr) == "hello");
      REQUIRE(adapter(ptr) != "world");
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
    struct type_custom
    {
      char* data = nullptr;
      std::size_t size = 0;
    };

    struct proxy
    {
      explicit proxy(type_custom& obj):
        obj_(obj)
      {}

      operator std::string() const
      {
        return std::string(obj_.data, obj_.size);
      }
      proxy& operator=(const std::string& rhs)
      {
        delete[] obj_.data;
        obj_.size = rhs.size()+1;
        obj_.data = new char[obj_.size];
        std::memcpy(obj_.data, rhs.data(), obj_.size);
        return *this;
      }
      bool operator==(const std::string& rhs) const
      {
        return std::strcmp(obj_.data, rhs.c_str()) == 0;
      }
      bool operator!=(const std::string& rhs) const
      {
        return !(*this == rhs);
      }

    private:
      type_custom& obj_;
    };
    static_assert(std::common_reference_with<proxy, std::string>);

    struct custom_reader
    {
      proxy operator()(type_custom& obj) const
      {
        return proxy(obj);
      }
    };
    static_assert(concepts::adaptable<type_custom&, custom_reader>);

    constexpr auto adapter = custom(custom_reader{});
    type_custom custom{};

    THEN("it implicitly assigns value")
    {
      adapter(custom) = "world";
      REQUIRE(adapter(custom) == std::string("world"));
    }
    THEN("it implicitly converts to type")
    {
      adapter(custom) = "hello";
      REQUIRE(adapter(custom) == "hello");
    }
    THEN("equality operator works")
    {
      adapter(custom) = "world";
      REQUIRE(adapter(custom) == "world");
      REQUIRE(adapter(custom) != "hello");
      REQUIRE("world" == adapter(custom));
      REQUIRE("hello" != adapter(custom));
    }
  }
}
