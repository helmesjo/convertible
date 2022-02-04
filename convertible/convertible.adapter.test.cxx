#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <array>
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
    GIVEN("member adapter")
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
            constexpr auto constexprAdapter2 = member(&type::val, deref());
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
