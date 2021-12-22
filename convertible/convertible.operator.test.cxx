#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <iostream> // Fix libc++ link error with doctest
#include <string>
#include <tuple>

TEST_CASE_TEMPLATE_DEFINE("it's invocable with types", arg_tuple_t, invocable_with_types)
{
    using operator_t = std::tuple_element_t<0, arg_tuple_t>;
    using lhs_t = std::tuple_element_t<1, arg_tuple_t>;
    using rhs_t = std::tuple_element_t<2, arg_tuple_t>;

    if constexpr(std::tuple_size_v<arg_tuple_t> > 3)
    {
        using converter_t = std::tuple_element_t<3, arg_tuple_t>;
        static_assert(std::invocable<operator_t, lhs_t, rhs_t, converter_t>);
    }
    else
    {
        static_assert(std::invocable<operator_t, lhs_t, rhs_t>);
    }
}

SCENARIO("convertible: Operators")
{
    using namespace convertible;

    GIVEN("assign operator")
    {
        struct int_string_converter
        {
            int operator()(std::string s)
            {
                return std::stoi(s);
            }
            std::string operator()(int i)
            {
                return std::to_string(i);
            }
        } converter;

        TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
            std::tuple<
                operators::assign,
                adapter::object<int&>&, 
                adapter::object<int&>&
            >,
            std::tuple<
                operators::assign,
                adapter::object<int&>&, 
                adapter::object<const int&>&
            >,
            std::tuple<
                operators::assign,
                adapter::object<int&&>&, 
                adapter::object<int&&>&
            >,
            std::tuple<
                operators::assign,
                adapter::object<int&&>&, 
                adapter::object<const int&>&
            >,
            std::tuple<
                operators::assign,
                adapter::object<int&>&, 
                adapter::object<std::string&>&,
                int_string_converter
            >
        );

        operators::assign op;

        std::vector<int> lhs = {0};
        std::vector<std::string> rhs = {"1"};
        op(lhs, rhs, converter);

        REQUIRE(lhs.size() == 1);
        REQUIRE(lhs[0] == 1);

        WHEN("passed two objects a & b")
        {
            int a = 1;
            int b = 2;

            THEN("a == b")
            {
                op(a, b);
                REQUIRE(a == b);
            }
            THEN("b is returned")
            {
                REQUIRE(&op(a, b) == &a);
            }
        }
        WHEN("passed two objects a & b (r-value)")
        {
            std::string a = "";
            std::string b = "hello";

            op(a, std::move(b));

            THEN("b is moved from")
            {
                REQUIRE(b == "");
            }
        }
    }

    GIVEN("equal operator")
    {
        TEST_CASE_TEMPLATE_INVOKE(invocable_with_types,
            std::tuple<
                operators::equal,
                const adapter::object<int&>&, 
                const adapter::object<int&>&
            >,
            std::tuple<
                operators::equal,
                const adapter::object<int&>&, 
                const adapter::object<const int&>&
            >,
            std::tuple<
                operators::equal,
                const adapter::object<const int&>&, 
                const adapter::object<const int&>&
            >,
            std::tuple<
                operators::equal,
                const adapter::object<int&&>&, 
                const adapter::object<int&&>&
            >,
            std::tuple<
                operators::equal,
                const adapter::object<int&&>&, 
                const adapter::object<const int&>&
            >
        );

        operators::equal op;
        WHEN("passed two equal objects a & b")
        {
            THEN("true is returned")
            {
                REQUIRE(op(1, 1));
            }
        }
        WHEN("passed two non-equal objects a & b")
        {
            THEN("false is returned")
            {
                REQUIRE_FALSE(op(1, 2));
            }
        }
    }
}
