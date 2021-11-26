#include <convertible/convertible.hxx>

#include <doctest/doctest.h>

#include <cstdint>
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define MSVC_ENUM_FIX(...) int
#else
#define MSVC_ENUM_FIX(...) __VA_ARGS__
#endif

template<typename T>
struct detect;

SCENARIO("convertible: Traits")
{
    using namespace convertible;

    struct type
    {
        int member;
    };

    static_assert(std::is_same_v<type, convertible::traits::member_class_t<decltype(&type::member)>>);
    static_assert(std::is_same_v<int, convertible::traits::member_value_t<decltype(&type::member)>>);
}

namespace tests
{
    struct mappable_type
    {
        template<MSVC_ENUM_FIX(convertible::direction) dir>
        void assign(int, int){}
    };
}

SCENARIO("convertible: Concepts")
{
    using namespace convertible;

    {
        struct type
        {
            int member;
        };

        static_assert(concepts::class_type<type>);
        static_assert(concepts::class_type<int> == false);
    }

    {
        struct type
        {
            type create(int){ return {}; }
            int create(std::string){ return {}; }
        };

        static_assert(concepts::adaptable<int, type>);
        static_assert(concepts::adaptable<std::string, type> == false);
    }

    {
        static_assert(concepts::mappable<tests::mappable_type, int, int>);
        static_assert(concepts::mappable<tests::mappable_type, int, std::string> == false);
    }
}

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

            std::string str = "hello";
            adapterRval = std::move(str);
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

        auto adapter = adapters::member(&type::str, obj);

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
}

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
                REQUIRE(converter(rhs) == lhs);
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

                AND_THEN("rhs is moved from")
                {
                    REQUIRE(rhs.val2 == "");
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
            table.assign<direction::lhs_to_rhs>(lhs_a, rhs_b);
            table.assign<direction::lhs_to_rhs>(lhs_a, rhs_c);

            THEN("a.val1 == b.val1")
            {
                REQUIRE(lhs_a.val1 == rhs_b.val1);
            }
            THEN("a.val1 == c.val1")
            {
                REQUIRE(lhs_a.val1 == rhs_c.val1);
            }
        }
    }
}