#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <string>

namespace
{
    struct int_string_converter
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

    auto verify_empty = [](auto&& rhs){
        return rhs == std::remove_cvref_t<decltype(rhs)>{};
    };

    template<typename verify_t = decltype(verify_empty)>
    void MAPS_CORRECTLY(auto&& lhs, auto&& rhs, const auto& map, verify_t verifyMoved = verify_empty)
    {
        using namespace convertible;
        
        WHEN("assigning rhs to lhs")
        {
            map.template assign<direction::rhs_to_lhs>(lhs, rhs);

            THEN("lhs == rhs")
            {
                REQUIRE(map.equal(lhs, rhs));
            }
        }
        WHEN("assigning rhs (r-value) to lhs")
        {
            map.template assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

            THEN("lhs is assigned")
            {
                AND_THEN("rhs is moved from")
                {
                    REQUIRE(verifyMoved(rhs));
                }
            }
        }
        WHEN("assigning lhs to rhs")
        {
            map.template assign<direction::lhs_to_rhs>(lhs, rhs);

            THEN("rhs == lhs")
            {
                REQUIRE(map.equal(rhs, lhs));
            }
        }
        WHEN("assigning lhs (r-value) to rhs")
        {
            map.template assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

            THEN("lhs is assigned")
            {
                AND_THEN("lhs is moved from")
                {
                    REQUIRE(verifyMoved(lhs));
                }
            }
        }
    }
}

SCENARIO("convertible: Mapping")
{
    using namespace convertible;

    THEN("it's constexpr constructible")
    {
        constexpr auto map = mapping(adapter::object(), adapter::object());
        (void)map;
    }
    GIVEN("a mapping between a <-> b")
    {
        adapter::object adapter = {};
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
        adapter::object adapter = {};
        constexpr int_string_converter converter{};
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

template<typename T>
struct detect;

SCENARIO("convertible: Mapping (misc use-cases)")
{
    using namespace convertible;

    constexpr int_string_converter converter{};

    GIVEN("vector<string> <-> vector<string>")
    {
        using lhs_t = std::vector<std::string>;
        using rhs_t = std::vector<std::string>;
        auto lhs = lhs_t{};
        auto rhs = rhs_t{};

        auto map = mapping(adapter::object(), adapter::object());

        lhs = {"1", "2", "3"};
        rhs = {"3", "2", "1"};
        MAPS_CORRECTLY(lhs, rhs, map);
    }
    GIVEN("vector<string> <-> vector<int>")
    {
        using lhs_t = std::vector<std::string>;
        using rhs_t = std::vector<int>;
        auto lhs = lhs_t{};
        auto rhs = rhs_t{};

        auto map = mapping(adapter::object(), adapter::object(), converter);

        lhs = {"1", "2", "3"};
        rhs = {1, 2, 3};
        MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
            if constexpr(std::same_as<lhs_t, std::decay_t<decltype(obj)>>)
                return obj[0] == "";
            else
                return obj[0] == 1;
        });
    }
}
