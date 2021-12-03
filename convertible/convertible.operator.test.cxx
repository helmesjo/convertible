#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <string>

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

            THEN("a == b")
            {
                op.exec(a, b);
                REQUIRE(a == b);
            }
            THEN("b is returned")
            {
                REQUIRE(&op.exec(a, b) == &a);
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
