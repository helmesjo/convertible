#include <convertible/convertible.hxx>

#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <vector>

template<typename T>
struct detect;

SCENARIO("convertible: Operators")
{
    using namespace convertible;

    GIVEN("assign operator")
    {
        operators::assign op;
        WHEN("passed two objects a & b")
        {
            int a = 1;
            int b = 2;
            auto returned = op.exec(a, b);

            THEN("a == b")
            {
                REQUIRE(a == b);

                AND_THEN("b is returned")
                {
                    REQUIRE(returned == b);
                }
            }
        }
        WHEN("passed two objects a & b (r-value)")
        {
            std::string a = "";
            std::string b = "hello";
            op.exec(a, std::move(b));

            THEN("b is moved from")
            {
                REQUIRE(b == "");
            }
        }
    }

    GIVEN("equal operator")
    {
        operators::equal op;
        WHEN("passed two equal objects a & b")
        {
            THEN("true is returned")
            {
                REQUIRE(op.exec(1, 1));
            }
        }
        WHEN("passed two non-equal objects a & b")
        {
            THEN("false is returned")
            {
                REQUIRE_FALSE(op.exec(1, 2));
            }
        }
    }
}

SCENARIO("convertible: Adapters")
{
    using namespace convertible;

    GIVEN("object adapter")
    {
        std::string str = "hello";
        adapters::object adapter(str);

        THEN("it implicitly assigns value")
        {
            adapter = "world";
            REQUIRE(str == "world");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == str);
        }
        THEN("it 'moves from' r-value reference")
        {
            adapters::object adapterRval(std::move(str));

            REQUIRE(static_cast<std::string>(adapterRval) == "hello");
            REQUIRE(str == "");
        }
        THEN("equality operator works")
        {
            str = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
    GIVEN("member adapter")
    {
        struct type
        {
            std::string str;
        } obj;

        adapters::member adapter(&type::str, obj);

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
            adapters::member adapterRval(&type::str, std::move(obj));

            REQUIRE(static_cast<std::string>(adapterRval) == "world");
            REQUIRE(obj.str == "");
        }
        THEN("equality operator works")
        {
            obj.str = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
}

SCENARIO("convertible: Mapping")
{
    using namespace convertible;

    GIVEN("a mapping between a <-> b")
    {
        adapters::object adapter;
        mapping map(adapter, adapter);

        WHEN("assigning lhs to rhs")
        {
            std::string lhs = "hello";
            std::string rhs = "";
            map.assign<direction::lhs_to_rhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(lhs == rhs);
            }
        }
        WHEN("assigning lhs (r-value) to rhs")
        {
            std::string lhs = "hello";
            std::string rhs = "";
            map.assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

            THEN("rhs is assigned")
            {
                REQUIRE(rhs == "hello");

                AND_THEN("lhs is moved from")
                {
                    REQUIRE(lhs == "");
                }
            }
        }
        WHEN("assigning rhs to lhs")
        {
            std::string lhs = "";
            std::string rhs = "hello";
            map.assign<direction::rhs_to_lhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(lhs == rhs);
            }
        }
        WHEN("assigning rhs (r-value) to lhs")
        {
            std::string lhs = "";
            std::string rhs = "hello";
            map.assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

            THEN("lhs is assigned")
            {
                REQUIRE(lhs == "hello");

                AND_THEN("rhs is moved from")
                {
                    REQUIRE(rhs == "");
                }
            }
        }
    }
    GIVEN("a mapping between a <- converter -> b")
    {
        struct custom_converter
        {
            int operator()(std::string val) const
            {
                return std::stoi(val);
            }

            std::string operator()(int val) const
            {
                return std::to_string(val);
            }
        };

        adapters::object adapter;
        constexpr custom_converter converter{};
        mapping map(adapter, adapter, converter);

        WHEN("assigning lhs to rhs")
        {
            int lhs = 11;
            std::string rhs = "";
            map.assign<direction::lhs_to_rhs>(lhs, rhs);

            THEN("converter(lhs) == rhs")
            {
                REQUIRE(converter(lhs) == rhs);
            }
        }
        WHEN("assigning lhs (r-value) to rhs")
        {
            std::string lhs = "11";
            int rhs = 0;
            map.assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

            THEN("rhs is assigned")
            {
                REQUIRE(rhs == 11);

                AND_THEN("lhs is moved from")
                {
                    REQUIRE(lhs == "");
                }
            }
        }
        WHEN("assigning rhs to lhs")
        {
            int lhs = 0;
            std::string rhs = "11";
            map.assign<direction::rhs_to_lhs>(lhs, rhs);

            THEN("converter(rhs) == lhs")
            {
                REQUIRE(converter(lhs) == rhs);
            }
        }
        WHEN("assigning rhs (r-value) to lhs")
        {
            int lhs = 0;
            std::string rhs = "11";
            map.assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

            THEN("lhs is assigned")
            {
                REQUIRE(lhs == 11);

                AND_THEN("rhs is moved from")
                {
                    REQUIRE(rhs == "");
                }
            }
        }
    }
}
