#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <iostream> // Fix libc++ link error with doctest
#include <string>

SCENARIO("convertible: Adapters")
{
    using namespace convertible;
    
    GIVEN("object adapter")
    {
        std::string str = "hello";
        auto adapter = adapters::object(str);

        // TODO: Test same for all adapters (parameterized tests?)
        THEN("it shares traits with held type")
        {            
            // l-value
            using adapter_ref_t = adapters::object<std::string&, adapters::readers::identity>;
            static_assert(std::convertible_to<adapter_ref_t, std::string>);
            static_assert(std::common_reference_with<adapter_ref_t, std::string>);
            static_assert(std::common_reference_with<std::string, adapter_ref_t>);
            static_assert(std::equality_comparable_with<std::string, adapter_ref_t>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string&>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string&&>);
            static_assert(std::assignable_from<adapter_ref_t&, const std::string&>);
            static_assert(std::assignable_from<adapter_ref_t&, const std::string&&>);
            static_assert(std::assignable_from<std::string&, adapter_ref_t>);

            // r-value
            using adapter_rref_t = adapters::object<std::string&&, adapters::readers::identity>;
            static_assert(std::common_reference_with<adapters::object<std::string>, adapters::object<const char*>>);
            static_assert(std::convertible_to<adapter_rref_t, std::string>);
            static_assert(std::convertible_to<adapter_rref_t, std::string&&>);
            static_assert(std::convertible_to<adapter_rref_t, const std::string&>);
            static_assert(std::convertible_to<adapter_rref_t, std::string&> == false);
            static_assert(std::common_reference_with<adapter_rref_t, std::string>);
            static_assert(std::common_reference_with<std::string, adapter_rref_t>);
            static_assert(std::equality_comparable_with<std::string, adapter_rref_t>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string&>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string&&>);
            static_assert(std::assignable_from<adapter_rref_t&, const std::string&>);
            static_assert(std::assignable_from<adapter_rref_t&, const std::string&&>);
            static_assert(std::assignable_from<std::string&, adapter_rref_t>);

            // misc
            static_assert(std::common_reference_with<adapter_ref_t, adapter_rref_t>);
            static_assert(std::common_reference_with<adapter_rref_t, adapters::object<const char*>>);
            static_assert(std::convertible_to<adapters::object<const char*>, std::string>);
            static_assert(std::convertible_to<const char*, std::string>);
        }
        THEN("it's adaptable only to the expected type(s)")
        {
            // (adapts to any type)
            static_assert(concepts::adaptable<std::string, decltype(adapter)>);
            static_assert(concepts::adaptable<int, decltype(adapter)>);
        }
        THEN("it implicitly assigns value")
        {
            adapter = "world";
            REQUIRE(str == "world");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == str);
        }
        THEN("implicit conversion 'moves from' r-value reference")
        {
            adapters::object adapterRval(std::move(str));

            std::string assigned = adapterRval;
            REQUIRE(assigned == "hello");
            REQUIRE(str == "");
        }
        THEN("assign 'moves from' r-value reference")
        {
            std::string str2 = "hello";
            adapter = std::move(str2);
            REQUIRE(str2 == "");
        }
        THEN("equality operator works")
        {
            str = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
            REQUIRE("world" == adapter);
            REQUIRE("hello" != adapter);
        }
    }
    GIVEN("member adapter")
    {
        struct type
        {
            std::string str;
        } obj;

        struct type_b
        {
            std::string str;
        } obj2;

        auto adapter = adapters::member(&type::str, obj);

        THEN("it shares traits with held type")
        {
            // l-value
            using adapter_ref_t = adapters::object<type&, adapters::readers::member<std::string type::*>>;
            static_assert(std::is_convertible_v<adapter_ref_t&, std::string&>);
            static_assert(std::is_convertible_v<const adapter_ref_t&, const std::string&>);
            static_assert(std::convertible_to<adapter_ref_t, std::string>);
            static_assert(std::common_reference_with<adapter_ref_t, std::string>);
            static_assert(std::common_reference_with<std::string, adapter_ref_t>);
            static_assert(std::equality_comparable_with<std::string, adapter_ref_t>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string&>);
            static_assert(std::assignable_from<adapter_ref_t&, std::string&&>);
            static_assert(std::assignable_from<adapter_ref_t&, const std::string&>);
            static_assert(std::assignable_from<adapter_ref_t&, const std::string&&>);
            static_assert(std::assignable_from<std::string&, adapter_ref_t>);

            // r-value
            using adapter_rref_t = adapters::object<type&&, adapters::readers::member<std::string type::*>>;
            static_assert(std::convertible_to<adapter_rref_t, std::string>);
            static_assert(std::convertible_to<adapter_rref_t, std::string&&>);
            static_assert(std::convertible_to<adapter_rref_t, const std::string&>);
            static_assert(std::convertible_to<adapter_rref_t, std::string&> == false);
            static_assert(std::common_reference_with<adapter_rref_t, std::string>);
            static_assert(std::common_reference_with<std::string, adapter_rref_t>);
            static_assert(std::equality_comparable_with<std::string, adapter_rref_t>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string&>);
            static_assert(std::assignable_from<adapter_rref_t&, std::string&&>);
            static_assert(std::assignable_from<adapter_rref_t&, const std::string&>);
            static_assert(std::assignable_from<adapter_rref_t&, const std::string&&>);
            static_assert(std::assignable_from<std::string&, adapter_rref_t>);

            // misc
            using adapter_b_ref_t = adapters::object<type_b&, adapters::readers::member<std::string type_b::*>>;
            static_assert(std::common_reference_with<adapter_ref_t, adapter_b_ref_t>);
            static_assert(std::assignable_from<adapter_ref_t&, adapter_b_ref_t>);
        }
        THEN("it's adaptable only to the expected type")
        {
            static_assert(concepts::adaptable<type, decltype(adapter)>);
            static_assert(concepts::adaptable<int, decltype(adapter)> == false);
        }
        THEN("it implicitly assigns member value")
        {
            adapter = "hello";
            REQUIRE(obj.str == "hello");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == obj.str);
        }
        THEN("it 'moves from' r-value reference")
        {
            obj.str = "world";

            auto adapterRval = adapters::member(&type::str, std::move(obj));
            REQUIRE(static_cast<std::string>(adapterRval) == "world");
            REQUIRE(obj.str == "");

            std::string str = "hello";
            adapterRval = std::move(str);
            REQUIRE(str == "");
        }
        THEN("equality operator works")
        {
            obj.str = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
    GIVEN("index adapter")
    {
        std::array values = {std::string("1")};
        auto adapter = adapters::index<0>(values);

        THEN("it's adaptable only to the expected type")
        {
            static_assert(concepts::adaptable<decltype(values), decltype(adapter)>);
            static_assert(concepts::adaptable<int, decltype(adapter)> == false);
        }
        THEN("it implicitly assigns member value")
        {
            adapter = "hello";
            REQUIRE(values[0] == "hello");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == values[0]);
        }
        THEN("it 'moves from' r-value reference")
        {
            values[0] = "world";

            auto adapterRval = convertible::adapters::index<0>(std::move(values));
            REQUIRE(static_cast<std::string>(adapterRval) == "world");
            REQUIRE(values[0] == "");

            std::string str = "hello";
            adapterRval = std::move(str);
            REQUIRE(str == "");
        }
        THEN("equality operator works")
        {
            values[0] = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
}
