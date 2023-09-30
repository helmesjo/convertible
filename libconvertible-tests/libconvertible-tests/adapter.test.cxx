#include <convertible/convertible.hxx>
#include <libconvertible-tests/test_common.hxx>
#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

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
    decltype(auto) operator()(auto&& arg) const
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

    THEN("it's constexpr constructible")
    {
      struct type_x{};
      static constexpr auto tmp = type_x{};
      static constexpr auto constexprAdapter = convertible::identity(tmp);
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
      WHEN("member function has an r-value overload")
      {
        struct type_rvalue_overload
        {
          std::string&& str() && { return std::move(str_); }
          std::string str_;
        } adaptee;
        adaptee.str_ = "hello";
        auto adapterRvalueFunc = member(&type_rvalue_overload::str);
        const std::string movedTo = adapterRvalueFunc(std::move(adaptee));
        REQUIRE(movedTo == "hello");
        REQUIRE(adaptee.str_ == "");
      }
      WHEN("member function has l-value overload it does NOT move")
      {
        const std::string movedTo = adapter(std::move(adaptee));
        REQUIRE(movedTo == "hello");
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

    THEN("it's constexpr constructible")
    {
      static constexpr auto tmp = std::array{""};
      static constexpr auto constexprAdapter = index<0>(tmp);
      (void)constexprAdapter;
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
  GIVEN("index adapter (string)")
  {
    auto adaptee = std::unordered_map<std::string, std::string>{{std::string("key"), std::string("hello")}};
    auto adapter = index<"key">(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee), decltype(adapter)>);
    static_assert(!concepts::adaptable<invalid_type, decltype(adapter)>);

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
      const std::string movedTo = adapter(std::move(adaptee));
      REQUIRE(movedTo == "hello");
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

    THEN("it returns defaulted/non-empty if adaptee is empty")
    {
      adaptee = std::nullopt;
      REQUIRE_FALSE(adaptee);
      REQUIRE(adapter(adaptee));
      REQUIRE(*adapter(adaptee) == "");
    }

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
  GIVEN("binary adapter")
  {
    struct non_trivially_copyable
    {
      virtual ~non_trivially_copyable() = default;
    };
    // binary reader (more specifically binary_proxy) is constructible from:
    // * trivially copyable types
    static_assert(requires{ reader::binary<0,3>::binary_proxy(std::declval<std::int32_t&>()); });
    static_assert(concepts::adaptable<int&, reader::binary<0, 1>>);
    static_assert(!concepts::adaptable<non_trivially_copyable&, reader::binary<0, 1>>);
    // * fixed size containers of trivially copyable types
    static_assert(requires{ reader::binary<0,3>::binary_proxy(std::declval<std::array<std::int32_t, 1>&>()); });
    // static_assert(concepts::adaptable<int[1], reader::binary<0, 1>>);
    static_assert(concepts::adaptable<std::array<int, 1>&, reader::binary<0, 0>>);
    static_assert(!concepts::adaptable<std::array<non_trivially_copyable, 1>&, reader::binary<0, 1>>);
    // * dynamic sequence containers of trivially copyable types
    static_assert(requires{ reader::binary<0,3>::binary_proxy(std::declval<std::vector<std::int32_t>&>()); });
    static_assert(concepts::adaptable<std::span<int>&, reader::binary<0, 1>>);
    static_assert(concepts::adaptable<std::vector<int>&, reader::binary<0, 1>>);
    static_assert(!concepts::adaptable<std::vector<non_trivially_copyable>&, reader::binary<0, 1>>);

    // binary reader is not constructible from too small type
    static_assert(!concepts::adaptable<std::int8_t&, reader::binary<0, sizeof(std::int32_t)>>);
    static_assert(!concepts::adaptable<std::array<std::int8_t, 1>&, reader::binary<0, sizeof(std::int32_t)>>);
    // but not if resizable
    static_assert(concepts::adaptable<std::vector<std::int8_t>&, reader::binary<0, sizeof(std::int32_t)>>);

    auto adaptee = std::uint32_t{};
    auto adapter = convertible::binary<0, 3>(adaptee);
    static_assert(concepts::adaptable<decltype(adaptee)&, decltype(adapter)>);

    WHEN("invoked with a range")
    {
      THEN("large enough fixed size range is valid")
      {
        static_assert(concepts::adaptable<std::array<std::int8_t, 4>&, reader::binary<0,1>>);
      }
      THEN("too small fixed size range is NOT valid")
      {
        static_assert(!concepts::adaptable<std::array<std::int8_t, 1>, reader::binary<0,3>>);
      }
      THEN("resizeable range is resized to fit all bytes")
      {
        // vector of int8
        {
          auto vectori8 = std::vector<std::int8_t>{};
          auto byteBuffer = adapter(vectori8);
          REQUIRE(vectori8.size() == 4);
        }
        // vector of int16
        {
          auto vectori16 = std::vector<std::int16_t>{};
          auto byteBuffer = adapter(vectori16);
          REQUIRE(vectori16.size() == 2);
        }
        // vector of int32
        {
          auto vectori32 = std::vector<std::int32_t>{};
          auto byteBuffer = adapter(vectori32);
          REQUIRE(vectori32.size() == 1);
        }
        // vector of int64
        {
          auto vectori64 = std::vector<std::int64_t>{};
          auto byteBuffer = adapter(vectori64);
          REQUIRE(vectori64.size() == 1);
        }

        WHEN("manually resized (made too small)")
        {
          auto vectori8 = std::vector<std::int8_t>{};
          auto byteBuffer = adapter(vectori8);
          vectori8.resize(0);
          THEN("it throws exception when assigned or compared to with a too big value")
          {
            REQUIRE_THROWS_AS(([&byteBuffer]{ byteBuffer = std::uint32_t{1}; }()), const std::overflow_error);
            REQUIRE_THROWS_AS(([&byteBuffer]{ (void)(byteBuffer == std::uint32_t{1}); }()), const std::overflow_error);
          }
        }
      }
      THEN("non-resizeable non-fixed size range throws exception")
      {
        auto vectori8 = std::vector<std::int8_t>{};
        auto emptySpan = std::span{vectori8};
        REQUIRE_THROWS_AS(([&adapter, &emptySpan]{ adapter(emptySpan); }()), const std::overflow_error);
      }
    }
    WHEN("invoked with a binary_proxy<storage_t, first1, last1>")
    {
      std::vector<std::uint32_t> storage;
      auto byteProxy = binary<0,3>()(binary<2,3>()(storage));
      THEN("it returns binary_proxy<storage_t, last1+1+first2, last1+1+first2+last2> (start one-past-last reusing the same storage)")
      {
        using reader_extended_t = decltype(byteProxy);
        static_assert(reader_extended_t::first_byte     == 4);
        static_assert(reader_extended_t::last_byte      == 7);
        static_assert(reader_extended_t::byte_count     == 4);
        static_assert(reader_extended_t::bytes_required == 8);
        static_assert(std::is_same_v<decltype(byteProxy.storage()), decltype(storage)&>);

        byteProxy = static_cast<std::uint32_t>(-1);
        REQUIRE(storage.size() == 2);
        REQUIRE(storage[0] == 0);
        REQUIRE(storage[1] == static_cast<std::uint32_t>(-1));
      }
    }

    THEN("it's constexpr constructible")
    {
      struct type_x{};
      static constexpr auto tmp = int{};
      static constexpr auto constexprAdapter = convertible::binary<0,1>(tmp);
      (void)constexprAdapter;
    }
    THEN("it implicitly assigns value")
    {
      // * trivially copyable types
      adapter(adaptee) = 8;
      REQUIRE(adaptee == 8);
      // * fixed size containers of trivially copyable types
      adapter(adaptee) = std::array{std::uint32_t{9}};
      REQUIRE(adaptee == 9);
      // * NOT dynamic sequence containers of trivially copyable types (assignment require known size)
      // adapter(adaptee) = std::vector{10};
      // REQUIRE(adaptee == 10);
    }
    THEN("it implicitly converts to type")
    {
      adaptee = 12;
      // * IMPLICIT: span of (const) bytes
      std::span<const std::byte> span = adapter(adaptee);
      REQUIRE(span[0] == std::byte{12});

      // * EXPLICIT: trivially copyable types
      int trivial = static_cast<int>(adapter(adaptee));
      REQUIRE(trivial == 12);
      int& trivialRef = static_cast<int&>(adapter(adaptee));
      REQUIRE(trivialRef == 12);
      // * EXPLICIT: fixed size containers of trivially copyable types (@DISABLED as UNSAFE)
      // auto fixed = static_cast<std::array<std::int32_t, 1>>(adapter(adaptee));
      // REQUIRE(fixed[0] == 12);
      // * EXPLICIT: dynamic sequence containers of trivially copyable types (@DISABLED as UNSAFE)
      // auto dynamic = static_cast<std::vector<int>>(adapter(adaptee));
      // REQUIRE(dynamic[0] == 12);
      // static_assert(concepts::range<std::vector<int>>);
    }
    THEN("equality operator works")
    {
      adaptee = 16;
      REQUIRE(adapter(adaptee) == 16);
      REQUIRE(adapter(adaptee) != "hel");
      REQUIRE(16 == adapter(adaptee));
      REQUIRE("hel" != adapter(adaptee));
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
      std::string& operator*() &
      {
        return val;
      }
      // support 'move from' deref
      std::string&& operator*() &&
      {
        return std::move(val);
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

  WHEN("adapter & adaptee are constexpr")
  {
    static constexpr int intVal = 1;
    static constexpr float floatVal = 1.0f;

    constexpr auto adapter = convertible::adapter();

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
