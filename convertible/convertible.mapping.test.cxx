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

            THEN("rhs is moved from")
            {
                REQUIRE(verifyMoved(rhs));
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

            THEN("lhs is moved from")
            {
                REQUIRE(verifyMoved(lhs));
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
        using lhs_t = std::string;
        using rhs_t = std::string;

        auto map = mapping(adapter::object(), adapter::object());

        auto lhs = lhs_t{"hello"};
        auto rhs = rhs_t{"world"};
        MAPS_CORRECTLY(lhs, rhs, map);
    }
    GIVEN("a mapping between a <- converter -> b")
    {
        using lhs_t = int;
        using rhs_t = std::string;

        auto map = mapping(adapter::object(), adapter::object(), int_string_converter{});

        auto lhs = lhs_t{11};
        auto rhs = rhs_t{"22"};
        MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
            if constexpr(std::same_as<lhs_t, std::decay_t<decltype(obj)>>)
                return obj == 11;
            else
                return obj == "";
        });
    }
}

SCENARIO("convertible: Mapping (misc use-cases)")
{
    using namespace convertible;

    GIVEN("enum_a <-> enum_b")
    {
        enum class enum_a{ val = 0 };
        enum class enum_b{ val = 0 };

        using lhs_t = enum_a;
        using rhs_t = enum_b;

        auto map = mapping(adapter::object(), adapter::object());

        auto lhs = lhs_t::val;
        auto rhs = rhs_t::val;
        MAPS_CORRECTLY(lhs, rhs, map, [](const auto&){
            return true;
        });
    }
    GIVEN("vector<string> <-> vector<string>")
    {
        using lhs_t = std::vector<std::string>;
        using rhs_t = std::vector<std::string>;

        auto map = mapping(adapter::object(), adapter::object());

        auto lhs = lhs_t{"1", "2", "3"};
        auto rhs = rhs_t{"3", "2", "1"};
        MAPS_CORRECTLY(lhs, rhs, map);
    }
    GIVEN("vector<string> <-> vector<int>")
    {
        using lhs_t = std::vector<std::string>;
        using rhs_t = std::vector<int>;

        auto map = mapping(adapter::object(), adapter::object(), int_string_converter{});

        auto lhs = lhs_t{"1", "2", "3"};
        auto rhs = rhs_t{1, 2, 3};
        MAPS_CORRECTLY(lhs, rhs, map, [](const auto& obj){
            if constexpr(std::same_as<lhs_t, std::decay_t<decltype(obj)>>)
                return std::all_of(std::begin(obj), std::end(obj), [](const auto& elem){ return elem == ""; });
            else
                return obj[0] == 1;
        });
    }
}
