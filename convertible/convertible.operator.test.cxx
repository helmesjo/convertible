#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <array>
#include <string>
#include <tuple>
#include <vector>

namespace
{
    auto verify_equal = [](const auto& lhs, const auto& rhs, const auto& converter){
        return lhs == converter(rhs);
    };

    auto verify_empty = [](auto&& rhs){
        return rhs == std::remove_cvref_t<decltype(rhs)>{};
    };
}

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

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity, typename verify_t = decltype(verify_equal)>
void COPY_ASSIGNS_CORRECTLY(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}, verify_t verifyEqual = verify_equal)
{
    CAPTURE(lhs);
    CAPTURE(rhs);
    CAPTURE(converter);

    auto op = convertible::operators::assign{};

    AND_WHEN("passed lhs & rhs")
    {
        THEN("lhs == rhs")
        {
            op(lhs, rhs, converter);
            REQUIRE(verifyEqual(lhs, rhs, converter));
        }
        THEN("lhs is returned")
        {
            REQUIRE(&op(lhs, rhs, converter) == &lhs);
        }
    }
}

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity, typename verify_t = decltype(verify_empty)>
void MOVE_ASSIGNS_CORRECTLY(lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {}, verify_t verifyMoved = verify_empty)
{
    CAPTURE(lhs);
    CAPTURE(rhs);
    CAPTURE(converter);

    auto op = convertible::operators::assign{};

    //static_assert(std::movable<std::decay_t<rhs_t>>, "rhs must be a movable type");
    
    AND_WHEN("passed lhs & rhs (r-value)")
    {
        op(lhs, std::move(rhs), converter);

        THEN("b is moved from")
        {
            REQUIRE(verifyMoved(rhs));
        }
    }
}

template<typename lhs_t, typename rhs_t, typename converter_t = convertible::converter::identity>
void EQUALITY_COMPARES_CORRECTLY(bool expectedResult, lhs_t&& lhs, rhs_t&& rhs, converter_t converter = {})
{
    CAPTURE(lhs);
    CAPTURE(rhs);
    CAPTURE(converter);

    auto op = convertible::operators::equal{};

    if(expectedResult)
    {
        THEN("lhs == rhs returns true")
        {
            REQUIRE(op(lhs, rhs, converter));
        }
    }
    else
    {
        THEN("lhs != rhs returns false")
        {
            REQUIRE_FALSE(op(lhs, rhs, converter));
        }
    }
}

SCENARIO("convertible: Operators")
{
    using namespace convertible;

    struct int_string_converter
    {
        int operator()(std::string s) const
        {
            return std::stoi(s);
        }
        std::string operator()(int i) const
        {
            return std::to_string(i);
        }
    } intStringConverter;

    enum class enum_a{};
    enum class enum_b{};

    GIVEN("assign operator")
    {
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
            >,
            std::tuple<
                operators::assign,
                adapter::object<std::vector<int>&>&, 
                adapter::object<std::vector<std::string>&>&,
                int_string_converter
            >,
            std::tuple<
                operators::assign,
                adapter::object<enum_a&>&, 
                adapter::object<enum_b&>&
            >
        );

        WHEN("lhs int, rhs int")
        {
            auto lhs = int{1};
            auto rhs = int{2};

            COPY_ASSIGNS_CORRECTLY(lhs, rhs);
        }

        WHEN("lhs string, rhs string")
        {
            auto lhs = std::string{"1"};
            auto rhs = std::string{"2"};

            COPY_ASSIGNS_CORRECTLY(lhs, rhs);
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
        }

        WHEN("lhs int, rhs string")
        {
            auto lhs = int{1};
            auto rhs = std::string{"2"};

            COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter);
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter);
        }

        WHEN("lhs vector<string>, rhs vector<string>")
        {
            auto lhs = std::vector<std::string>{};
            auto rhs = std::vector<std::string>{ "2" };

            COPY_ASSIGNS_CORRECTLY(lhs, rhs);
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
        }

        WHEN("lhs vector<int>, rhs vector<string>")
        {
            auto lhs = std::vector<int>{};
            auto rhs = std::vector<std::string>{ "2" };

            COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
                return lhs[0] == converter(rhs[0]);
            });
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
                return rhs[0] == "";
            });
        }

        WHEN("lhs array<string>, rhs array<string>")
        {
            auto lhs = std::array<std::string, 1>{};
            auto rhs = std::array<std::string, 1>{ "2" };

            COPY_ASSIGNS_CORRECTLY(lhs, rhs);
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs));
        }

        WHEN("lhs array<int>, rhs array<string>")
        {
            auto lhs = std::array<int, 1>{};
            auto rhs = std::array<std::string, 1>{ "2" };

            COPY_ASSIGNS_CORRECTLY(lhs, rhs, intStringConverter, [](const auto& lhs, const auto& rhs, const auto& converter){
                return lhs[0] == converter(rhs[0]);
            });
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), intStringConverter, [](const auto& rhs){
                return rhs[0] == "";
            });
        }

        WHEN("lhs array<string>, rhs vector<string>")
        {
            auto lhs = std::array<std::string, 1>{};
            auto rhs = std::vector<std::string>{ "2" };

            COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                return lhs[0] == converter(rhs[0]);
            });
            MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                return rhs[0] == "";
            });
        }

        WHEN("lhs is dynamic container, rhs is dynamic container")
        {
            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::vector<std::string>{ "5" };
                auto rhs = std::vector<std::string>{ "1", "2" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs == converter(rhs);
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs.empty();
                });
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::vector<std::string>{ "5", "6" };
                auto rhs = std::vector<std::string>{ "1" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs == converter(rhs);
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs.empty();
                });
            }
        }

        WHEN("lhs is static container, rhs is dynamic container")
        {
            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::array<std::string, 1>{};
                auto rhs = std::vector<std::string>{ "1", "2" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs[0] == converter(rhs[0]);
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs[0] == "" && rhs[1] == "2";
                });
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::array<std::string, 2>{"5", "6"};
                auto rhs = std::vector<std::string>{ "1" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs[0] == converter(rhs[0]) && lhs[1] == "6";
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs[0] == "";
                });
            }
        }

        WHEN("lhs is dynamic container, rhs is static container")
        {
            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::vector<std::string>{ "1" };
                auto rhs = std::array<std::string, 2>{ "1", "2" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs.size() == 2 && lhs[0] == converter(rhs[0]) && lhs[1] == converter(rhs[1]);
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs[0] == "" && rhs[1] == "";
                });
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::vector<std::string>{ "1", "2" };
                auto rhs = std::array<std::string, 1>{ "5" };

                COPY_ASSIGNS_CORRECTLY(lhs, rhs, converter::identity{}, [](const auto& lhs, const auto& rhs, const auto& converter){
                    return lhs.size() == 1 && lhs[0] == converter(rhs[0]);
                });
                MOVE_ASSIGNS_CORRECTLY(lhs, std::move(rhs), converter::identity{}, [](const auto& rhs){
                    return rhs[0] == "";
                });
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
            >,
            std::tuple<
                operators::equal,
                adapter::object<int&>&, 
                adapter::object<std::string&>&,
                int_string_converter
            >,
            std::tuple<
                operators::equal,
                adapter::object<std::vector<int>&>&, 
                adapter::object<std::vector<std::string>&>&,
                int_string_converter
            >,
            std::tuple<
                operators::equal,
                adapter::object<enum_a&>&, 
                adapter::object<enum_b&>&
            >
        );

        WHEN("lhs int, rhs int")
        {
            auto lhs = int{1};
            auto rhs = int{1};

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
            rhs = 2;
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
        }

        WHEN("lhs string, rhs string")
        {
            auto lhs = std::string{"hello"};
            auto rhs = std::string{"hello"};

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
            rhs = "world";
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
        }

        WHEN("lhs int, rhs string")
        {
            auto lhs = int{1};
            auto rhs = std::string{"1"};

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
            rhs = "2";
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
        }
        
        WHEN("lhs vector<string>, rhs vector<string>")
        {
            auto lhs = std::vector<std::string>{ "hello" };
            auto rhs = std::vector<std::string>{ "hello" };

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
            rhs = { "world" };
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
        }
        
        WHEN("lhs vector<int>, rhs vector<string>")
        {
            auto lhs = std::vector<int>{ 1 };
            auto rhs = std::vector<std::string>{ "1" };

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
            rhs = { "2" };
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
        }

        WHEN("lhs array<string>, rhs array<string>")
        {
            auto lhs = std::array<std::string, 1>{ "1" };
            auto rhs = std::array<std::string, 1>{ "1" };

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
            rhs = { "2" };
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
        }

        WHEN("lhs array<int>, rhs array<string>")
        {
            auto lhs = std::array<int, 1>{ 1 };
            auto rhs = std::array<std::string, 1>{ "1" };

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs, intStringConverter);
            rhs = { "2" };
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs, intStringConverter);
        }

        WHEN("lhs array<string>, rhs vector<string>")
        {
            auto lhs = std::array<std::string, 1>{ "1" };
            auto rhs = std::vector<std::string>{ "1" };

            EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
            rhs = { "2" };
            EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
        }

        WHEN("lhs is dynamic container, rhs is dynamic container")
        {
            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::vector<std::string>{ "1" };
                auto rhs = std::vector<std::string>{ "1", "2" };

                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::vector<std::string>{ "5", "6" };
                auto rhs = std::vector<std::string>{ "5" };

                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }
        }

        WHEN("lhs is static container, rhs is dynamic container")
        {
            AND_WHEN("lhs size == rhs size")
            {
                auto lhs = std::array<std::string, 2>{ "1", "2" };
                auto rhs = std::vector<std::string>{ "1", "2" };

                EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
                rhs = { "2", "1" };
                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }

            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::array<std::string, 1>{ "1" };
                auto rhs = std::vector<std::string>{ "1", "2" };

                EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
                rhs = { "2", "1" };
                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::array<std::string, 2>{"5", "6"};
                auto rhs = std::vector<std::string>{ "5" };

                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }
        }

        WHEN("lhs is dynamic container, rhs is static container")
        {
            AND_WHEN("lhs size == rhs size")
            {
                auto lhs = std::vector<std::string>{ "1", "2" };
                auto rhs = std::array<std::string, 2>{ "1", "2" };

                EQUALITY_COMPARES_CORRECTLY(true, lhs, rhs);
                rhs = { "2", "1" };
                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }

            AND_WHEN("lhs size < rhs size")
            {
                auto lhs = std::vector<std::string>{ "1" };
                auto rhs = std::array<std::string, 2>{ "1", "2" };

                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }

            AND_WHEN("lhs size > rhs size")
            {
                auto lhs = std::vector<std::string>{ "1", "2" };
                auto rhs = std::array<std::string, 2>{ "1" };

                EQUALITY_COMPARES_CORRECTLY(false, lhs, rhs);
            }
        }
    }
}
