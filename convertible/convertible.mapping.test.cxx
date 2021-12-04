#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <iostream> // Fix libc++ link error with doctest
#include <string>

SCENARIO("convertible: Mapping")
{
    using namespace convertible;

    GIVEN("a mapping between a <-> b")
    {
        adapters::object adapter = {};
        mapping map(adapter, adapter);

        WHEN("assigning lhs to rhs")
        {
            std::string lhs = "hello";
            std::string rhs = "";
            map.assign<direction::lhs_to_rhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(lhs == rhs);
                REQUIRE(map.equal(lhs, rhs));
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
                REQUIRE(map.equal(lhs, rhs));
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
                    REQUIRE(map.equal(rhs, ""));
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
                try
                {
                    return std::stoi(val);
                }
                catch(const std::exception&)
                {
                    return 0;
                }
            }

            std::string operator()(int val) const
            {
                return std::to_string(val);
            }
        };

        adapters::object adapter = {};
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
                REQUIRE(map.equal(lhs, rhs));
                REQUIRE(map.equal(rhs, lhs));
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
                    REQUIRE_FALSE(map.equal(lhs, rhs));
                    REQUIRE_FALSE(map.equal(rhs, lhs));
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
                REQUIRE(converter(rhs) == lhs);
                REQUIRE(map.equal(lhs, rhs));
                REQUIRE(map.equal(rhs, lhs));
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
                    REQUIRE_FALSE(map.equal(lhs, rhs));
                    REQUIRE_FALSE(map.equal(rhs, lhs));
                }
            }
        }
    }
}
