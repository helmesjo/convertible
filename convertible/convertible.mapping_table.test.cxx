#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <iostream> // Fix libc++ link error with doctest
#include <string>

SCENARIO("convertible: Mapping table")
{
    using namespace convertible;

    struct type_a
    {
        int val1;
        std::string val2;
    };

    struct type_b
    {
        int val1;
        std::string val2;
    };

    struct type_c
    {
        int val1;
    };

    GIVEN("mapping table between \n\n\ta.val1 <-> b.val1\n\ta.val2 <-> b.val2\n")
    {
        mapping_table table{
            mapping( adapters::member(&type_a::val1), adapters::member(&type_b::val1) ),
            mapping( adapters::member(&type_a::val2), adapters::member(&type_b::val2) )
        };

        type_a lhs;
        type_b rhs;

        WHEN("assigning lhs to rhs")
        {
            lhs.val1 = 10;
            lhs.val2 = "hello";
            table.assign<direction::lhs_to_rhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(lhs.val1 == rhs.val1);
                REQUIRE(lhs.val2 == rhs.val2);
                REQUIRE(table.equal(lhs, rhs));
                REQUIRE(table.equal(rhs, lhs));
            }
        }
        WHEN("assigning lhs (r-value) to rhs")
        {
            lhs.val1 = 10;
            lhs.val2 = "hello";
            table.assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

            THEN("rhs is assigned")
            {
                REQUIRE(rhs.val1 == 10);
                REQUIRE(rhs.val2 == "hello");

                AND_THEN("lhs is moved from")
                {
                    REQUIRE(lhs.val2 == "");
                    REQUIRE_FALSE(table.equal(lhs, rhs));
                }
            }
        }
        WHEN("assigning rhs to lhs")
        {
            rhs.val1 = 10;
            rhs.val2 = "hello";
            table.assign<direction::rhs_to_lhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(lhs.val1 == rhs.val1);
                REQUIRE(lhs.val2 == rhs.val2);
                REQUIRE(table.equal(lhs, rhs));
                REQUIRE(table.equal(rhs, lhs));
            }
        }
        WHEN("assigning rhs (r-value) to lhs")
        {
            rhs.val1 = 10;
            rhs.val2 = "hello";
            table.assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

            THEN("lhs is assigned")
            {
                REQUIRE(lhs.val1 == 10);
                REQUIRE(lhs.val2 == "hello");
                REQUIRE(table.equal(lhs, type_a{10, "hello"}));

                AND_THEN("rhs is moved from")
                {
                    REQUIRE(rhs.val2 == "");
                    REQUIRE_FALSE(table.equal(lhs, rhs));
                }
            }
        }
    }

    GIVEN("mapping table between \n\n\ta.val1 <-> b.val1\n\ta.val1 <-> c.val1\n")
    {
        struct type_c
        {
            int val1;
        };

        mapping_table table{
            mapping( adapters::member(&type_a::val1), adapters::member(&type_b::val1) ),
            mapping( adapters::member(&type_a::val1), adapters::member(&type_c::val1) )
        };

        type_a lhs_a;
        type_b rhs_b;
        type_c rhs_c;

        WHEN("assigning lhs (a) to rhs (b)")
        {
            lhs_a.val1 = 10;
            lhs_a.val2 = "hello";
            table.assign<direction::lhs_to_rhs>(lhs_a, rhs_b);

            THEN("a.val1 == b.val1")
            {
                REQUIRE(lhs_a.val1 == rhs_b.val1);
                REQUIRE(table.equal(lhs_a, rhs_b));
                REQUIRE(table.equal(rhs_b, lhs_a));
            }
        }
        WHEN("assigning lhs (a) to rhs (c)")
        {
            lhs_a.val1 = 10;
            lhs_a.val2 = "hello";
            table.assign<direction::lhs_to_rhs>(lhs_a, rhs_c);

            THEN("a.val1 == c.val1")
            {
                REQUIRE(lhs_a.val1 == rhs_c.val1);
                REQUIRE(table.equal(lhs_a, rhs_c));
                REQUIRE(table.equal(rhs_c, lhs_a));
            }
        }
    }
}
