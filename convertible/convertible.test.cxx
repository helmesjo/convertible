#include <convertible/convertible.hxx>

#include <doctest/doctest.h>

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

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
        adapters::object<std::string&> adapter(str);

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
    }
    GIVEN("member adapter")
    {
        struct type
        {
            int val = 1;
        } obj;

        adapters::member adapter(&type::val, obj);

        THEN("it implicitly assigns member value")
        {
            adapter = 3;
            REQUIRE(obj.val == 3);
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<int>(adapter) == obj.val);
        }
    }
}

SCENARIO("playground1")
{
    using namespace convertible;

    std::uint32_t val1, val2 = 0;
    adapters::object adapter1;
    adapters::object adapter2;

    // operators::assign op1;
    // operators::compare op2;

    // val1 = 1;
    // val2 = 2;
    // assign(adapter1, val1, adapter2, 2);
    // REQUIRE(val1 == 2);
    // REQUIRE(compare(adapter1, val1, adapter2, 2));

    //op1.exec(adapter1, 1, adapter2, 2);
    // op2.exec(adapter1, adapter2);

    // mapping = map(member(&type::mbr), converter, int);

    // op1.exec(mapping, type, int);

    struct dummy
    {
        int val = 0;
    } obj1, obj2;
    adapters::member mbr1(&dummy::val);
    adapters::member mbr2(&dummy::val);

    obj1.val = 1;
    obj2.val = 2;
    // assign(mbr1, obj1, mbr2, std::move(obj2));
    // assign(mbr1, obj1, adapter2, 2);
    // REQUIRE(obj1.val == obj2.val);
    // REQUIRE(compare(mbr1, obj1, mbr2, std::move(obj2)));

    mapping m(mbr1, adapter2);

    m.assign<direction::rhs_to_lhs>(obj1, 5);
    REQUIRE(obj1.val == 5);
    m.assign<direction::lhs_to_rhs>(obj1, val1);
    REQUIRE(val1 == 5);
    REQUIRE(m.equal(obj1, 5));

    mapping m2(adapter1, mbr2);
    REQUIRE(m2.equal(val1, obj1));
}

// SCENARIO("playground1.1")
// {
//     std::uint32_t val1, val2 = 0;
//     adapters::object_v2 adapter1;
//     adapters::object_v2 adapter2;

//     operators::assign_v2 op1;
//     operators::compare_v2 op2;

//     op1.exec(adapter1, adapter2);
//     op2.exec(adapter1, adapter2);
// }

// SCENARIO("playground2")
// {
//     std::uint32_t val1, val2 = 0;
//     adapters::object adapter1(val1);
//     adapters::object adapter2(val2);

//     operators::assign op1;
//     operators::compare op2;

//     val2 = 5;
//     REQUIRE_FALSE(op2.exec(adapter1, adapter2));
//     REQUIRE(op1.exec(adapter1, adapter2) == 5);
//     REQUIRE(op2.exec(adapter1, adapter2));
// }

// SCENARIO("playground3")
// {
//     struct dummy
//     {
//         int val = 0;
//     } obj;
//     adapters::member mbrAdptr(obj, &dummy::val);

//     std::uint32_t val1 = 1;
//     adapters::object objAdptr(val1);

//     operators::compare comp;

//     REQUIRE_FALSE(comp.exec(objAdptr, mbrAdptr));
// }

// SCENARIO("playground3")
// {
//     std::vector<std::uint8_t> data;

//     mapping<adapter, adapter> m;

//     std::uint32_t a1=1, a2=2;
//     m.assign<direction::lhs_to_rhs>(a1, a2);

//     REQUIRE(a1 == 1);
//     REQUIRE(a2 == 1);
//     REQUIRE(m.equals(a1, a2));

//     a1=1;
//     a2=2;

//     m.assign<direction::rhs_to_lhs>(a1, a2);

//     REQUIRE(a1 == 2);
//     REQUIRE(a2 == 2);
//     REQUIRE(m.equals(a1, a2));
// }
