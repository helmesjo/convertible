#include <convertible/convertible.hxx>

#include <catch2/catch.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace
{    
    template<typename destination_t, typename source_t, typename converter_t = int>
    void ASSIGN_AND_VERIFY(destination_t&& destination, source_t&& source, converter_t&& converter)
    {
        auto sourceCopy = source;

        if constexpr(!std::is_same_v<std::decay_t<converter_t>, int>)
        {
            convertible::values::assign(FWD(destination), FWD(source), converter);
            REQUIRE(convertible::values::equal(destination, sourceCopy, converter));
        }
        else
        {
            convertible::values::assign(FWD(destination), FWD(source));
            REQUIRE(convertible::values::equal(destination, sourceCopy));
        }
    }

    template<typename destination_t, typename source_t, typename converter_t = int>
    void TEST_COPY_ASSIGNMENT(destination_t&& dest, source_t source, converter_t converter = {})
    {
        WHEN("copy assigned")
        {
            // Verify assignment
            THEN("destination is assigned")
            {
                auto sourceCopy = source;
                ASSIGN_AND_VERIFY(FWD(dest), source, converter);

                // Verify perfect forwarding
                AND_THEN("source is NOT 'moved from'")
                {
                    REQUIRE(source == sourceCopy);
                }
            }
        }
    }

    template<typename destination_t, typename source_t, typename converter_t = int>
    void TEST_COPY_ASSIGNMENT(source_t source, converter_t converter = {})
    {
        destination_t dest;
        TEST_COPY_ASSIGNMENT(dest, source, converter);
    }

    template<typename destination_t, typename source_t, typename converter_t = int>
    void TEST_MOVE_ASSIGNMENT(source_t source, converter_t converter = {})
    {
        WHEN("move assigned")
        {
            THEN("destination is assigned")
            {
                destination_t dest;
                ASSIGN_AND_VERIFY(dest, std::move(source), converter);

                // Verify perfect forwarding
                using source_decayed_t = std::decay_t<source_t>;
                AND_THEN("source is 'moved from'")
                {
                    REQUIRE(source == source_decayed_t{});
                }
            }
        }
    }

    struct int_string_converter_t
    {
        int operator()(std::string in)
        {
            return std::atoi(in.c_str());
        }
        std::string operator()(int in)
        {
            return std::to_string(in);
        }
    } int_string_converter;

    constexpr auto int_to_string_converter = [](int input) -> std::string
    {
        return std::to_string(input);
    };
    using int_to_string_converter_t = decltype(int_to_string_converter);
}

SCENARIO("convertible_tests: Compile-time validation")
{
    GIVEN("misc traits")
    {
        static_assert(convertible::traits::is_dereferencable_v<int*>);
        static_assert(convertible::traits::is_dereferencable_v<std::optional<int>>);
        static_assert(!convertible::traits::is_dereferencable_v<int>);
    }
    GIVEN("conversion")
    {
        // True:
        static_assert(std::is_same_v<std::string, convertible::traits::converted_t<int_to_string_converter_t, int>>);
        static_assert(convertible::traits::is_convertible_v<int_to_string_converter_t, int>);
        static_assert(convertible::traits::is_convertible_v<int_to_string_converter_t, int&>);
        static_assert(convertible::traits::is_convertible_v<int_to_string_converter_t, float>);

        // False:
        static_assert(!convertible::traits::is_convertible_v<int_to_string_converter_t, std::string>);
    }
    GIVEN("direct assignment")
    {
        // True:
        // Without converter
        static_assert(convertible::traits::is_assignable_v<int&, int>);
        static_assert(convertible::traits::is_assignable_v<int&, int&>);
        static_assert(convertible::traits::is_assignable_v<int&, float>);
        static_assert(convertible::traits::is_assignable_v<float&, int>);
        static_assert(convertible::traits::is_assignable_v<std::string, std::string>);
        static_assert(convertible::traits::is_assignable_v<std::vector<int>, std::vector<int>>);
        // With converter
        static_assert(convertible::traits::is_assignable_v<std::string&, int, int_to_string_converter_t>);

        // False:
        // Without converter
        static_assert(!convertible::traits::is_assignable_v<int, int>);
        static_assert(!convertible::traits::is_assignable_v<int, int&>);
        static_assert(!convertible::traits::is_assignable_v<std::string, int>);
        static_assert(!convertible::traits::is_assignable_v<std::vector<int>, std::vector<float>>);
        // With converter
        static_assert(!convertible::traits::is_assignable_v<std::vector<int>, std::vector<float>, int_to_string_converter_t>);
        static_assert(!convertible::traits::is_assignable_v<int&, std::string, int_to_string_converter_t>);
    }
    GIVEN("direct comparison")
    {
        // True:
        // Without converter
        static_assert(convertible::traits::is_comparable_v<int, int>);
        static_assert(convertible::traits::is_comparable_v<int, int&>);
        static_assert(convertible::traits::is_comparable_v<int&, int>);
        static_assert(convertible::traits::is_comparable_v<int&, int&>);
        static_assert(convertible::traits::is_comparable_v<int&, float>);
        static_assert(convertible::traits::is_comparable_v<float&, int>);
        static_assert(convertible::traits::is_comparable_v<std::string, std::string>);
        static_assert(convertible::traits::is_comparable_v<std::vector<int>, std::vector<int>>);
        // With converter
        static_assert(convertible::traits::is_comparable_v<std::string&, int, int_to_string_converter_t>);

        // False:
        // Without converter
        static_assert(!convertible::traits::is_comparable_v<std::string, int>);
        static_assert(!convertible::traits::is_comparable_v<std::vector<int>, std::vector<float>>);
        // With converter
        static_assert(!convertible::traits::is_comparable_v<std::vector<int>, std::vector<float>, int_to_string_converter_t>);
        static_assert(!convertible::traits::is_comparable_v<int&, std::string, int_to_string_converter_t>);
    }
}

SCENARIO("convertible_tests: Assignment & Equality")
{
    GIVEN("int -> int")
    {
        WHEN("assigning")
        {
            using dest_t = int;
            using source_t = int;
            TEST_COPY_ASSIGNMENT<dest_t, source_t>(10);
        }
        WHEN("comparing")
        {
            using lhs_t = int;
            using rhs_t = int;
            REQUIRE(convertible::values::equal(lhs_t{1}, rhs_t{1}));
            REQUIRE_FALSE(convertible::values::equal(lhs_t{1}, rhs_t{2}));
        }
    }
    GIVEN("int <-> int*")
    {
        WHEN("assigning")
        {
            AND_WHEN("int -> int*")
            {
                using dest_t = int*;
                using source_t = int;

                int val;
                dest_t dest = &val;
                TEST_COPY_ASSIGNMENT(dest, source_t{10});
            }
            AND_WHEN("int -> int* (nullptr)")
            {
                using dest_t = int*;
                using source_t = int;

                dest_t destination = nullptr;
                REQUIRE_NOTHROW(convertible::values::assign(destination, source_t{10}));
                REQUIRE(destination == nullptr);
            }
            AND_WHEN("int <- int*")
            {
                using dest_t = int;
                using source_t = int*;
                int source = 10;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>(&source);
            }
            AND_WHEN("int <- int* (nullptr)")
            {
                using dest_t = int;
                using source_t = int*;
                int destination = 10;
                REQUIRE_NOTHROW(convertible::values::assign(destination, source_t{ nullptr }));
                REQUIRE(destination == 10);
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("int == int*")
            {
                using lhs_t = int;
                using rhs_t = int*;

                int rhs = 12;
                REQUIRE(convertible::values::equal(lhs_t{12}, &rhs));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{2}, &rhs));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{2}, rhs_t{nullptr}));
            }
            AND_WHEN("int* == int")
            {
                using lhs_t = int*;
                using rhs_t = int;
                int lhs = 12;
                REQUIRE(convertible::values::equal(&lhs, rhs_t{12}));
                REQUIRE_FALSE(convertible::values::equal(&lhs, rhs_t{2}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{nullptr}, rhs_t{2}));
            }
        }
    }
    GIVEN("int <-> float")
    {
        WHEN("assigning")
        {
            AND_WHEN("float -> int")
            {
                using dest_t = int;
                using source_t = float;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>(10.3f);
            }
            AND_WHEN("float <- int")
            {
                using dest_t = float;
                using source_t = int;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>(10);
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("int == float")
            {
                using lhs_t = int;
                using rhs_t = float;
                REQUIRE(convertible::values::equal(lhs_t{1}, rhs_t{1.0f}));
                REQUIRE(convertible::values::equal(lhs_t{1}, rhs_t{1.4f}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1}, rhs_t{2.0f}));
            }
            AND_WHEN("float == int")
            {
                using lhs_t = float;
                using rhs_t = int;
                REQUIRE(convertible::values::equal(lhs_t{1.0f}, rhs_t{1}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1.4f}, rhs_t{1}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{2.0f}, rhs_t{1}));
            }
        }
    }
    GIVEN("string -> string")
    {
        WHEN("assigning")
        {
            using dest_t = std::string;
            using source_t = std::string;
            TEST_COPY_ASSIGNMENT<dest_t, source_t>("source");
            TEST_MOVE_ASSIGNMENT<dest_t, source_t>("source");
        }
        WHEN("comparing")
        {
            using lhs_t = std::string;
            using rhs_t = std::string;
            REQUIRE(convertible::values::equal(lhs_t{"hello"}, rhs_t{"hello"}));
            REQUIRE_FALSE(convertible::values::equal(lhs_t{"hello"}, rhs_t{"world"}));
        }
    }
    GIVEN("array<int, 3> -> array<int, 3>")
    {
        WHEN("assigning")
        {
            using dest_t = std::array<int, 3>;
            using source_t = std::array<int, 3>;
            TEST_COPY_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
        }
        WHEN("comparing")
        {
            using lhs_t = std::array<int, 3>;
            using rhs_t = std::array<int, 3>;
            REQUIRE(convertible::values::equal(lhs_t{1, 2}, rhs_t{1, 2}));
            REQUIRE_FALSE(convertible::values::equal(lhs_t{1, 2}, rhs_t{3, 4}));
        }
    }
    GIVEN("array<int, 3> <-> array<float, 3>")
    {
        WHEN("assigning")
        {
            AND_WHEN("array<int, 3> -> array<float, 3>")
            {
                using dest_t = std::array<float, 3>;
                using source_t = std::array<int, 3>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
            }
            AND_WHEN("array<int, 3> <- array<float, 3>")
            {
                using dest_t = std::array<int, 3>;
                using source_t = std::array<float, 3>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({1.1f, 2.2f, 3.3f});
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("array<int, 3> == array<float, 3>")
            {
                using lhs_t = std::array<int, 3>;
                using rhs_t = std::array<float, 3>;
                REQUIRE(convertible::values::equal(lhs_t{1, 2, 3}, rhs_t{1.0f, 2.0f, 3.0f}));
                REQUIRE(convertible::values::equal(lhs_t{1, 2, 3}, rhs_t{1.4f, 2.6f, 3.5f}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1, 2, 3}, rhs_t{2.0f, 2.0f, 2.0f}));
            }
            AND_WHEN("array<float, 3> == array<int, 3>")
            {
                using lhs_t = std::array<float, 3>;
                using rhs_t = std::array<int, 3>;
                REQUIRE(convertible::values::equal(lhs_t{1.0f, 2.0f, 3.0f}, rhs_t{1, 2, 3}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1.4f, 2.6f, 3.5f}, rhs_t{1, 2, 3}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{2.0f, 2.0f, 3}, rhs_t{1, 2, 3}));
            }
        }
    }
    GIVEN("array<int, 3> <-> vector<float, 3>")
    {
        WHEN("assigning")
        {
            AND_WHEN("array<int, 3> -> vector<float>")
            {
                using dest_t = std::vector<float>;
                using source_t = std::array<int, 3>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({ 1, 2, 3 });
                dest_t lhs;
                convertible::values::assign(lhs, source_t{ 1, 2, 3 }); // Compile-time check that fixed-size source "moved from" isn't cleared (`rhs.clear()` called)
            }
            AND_WHEN("array<int, 3> <- vector<float>")
            {
                using dest_t = std::array<int, 3>;
                using source_t = std::vector<float>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({ 1.1f, 2.2f, 3.3f });
                TEST_MOVE_ASSIGNMENT<dest_t, source_t>({ 1.1f, 2.2f, 3.3f });
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("array<int, 3> == vector<float>")
            {
                using lhs_t = std::array<int, 3>;
                using rhs_t = std::vector<float>;
                REQUIRE(convertible::values::equal(lhs_t{ 1, 2, 3 }, rhs_t{ 1.0f, 2.0f, 3.0f }));
                REQUIRE(convertible::values::equal(lhs_t{ 1, 2, 3 }, rhs_t{ 1.4f, 2.6f, 3.5f }));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{ 1, 2, 3 }, rhs_t{ 2.0f, 2.0f, 2.0f }));
            }
            AND_WHEN("vector<float> == array<int, 3>")
            {
                using lhs_t = std::vector<float>;
                using rhs_t = std::array<int, 3>;
                REQUIRE(convertible::values::equal(lhs_t{ 1.0f, 2.0f, 3.0f }, rhs_t{ 1, 2, 3 }));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{ 1.4f, 2.6f, 3.5f }, rhs_t{ 1, 2, 3 }));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{ 2.0f, 2.0f, 3 }, rhs_t{ 1, 2, 3 }));
            }
        }
    }
    GIVEN("vector<int> -> vector<int>")
    {
        WHEN("assigning")
        {
            using dest_t = std::vector<int>;
            using source_t = std::vector<int>;
            TEST_COPY_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
            TEST_MOVE_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
        }
        WHEN("comparing")
        {
            using lhs_t = std::vector<int>;
            using rhs_t = std::vector<int>;
            REQUIRE(convertible::values::equal(lhs_t{1, 2}, rhs_t{1, 2}));
            REQUIRE_FALSE(convertible::values::equal(lhs_t{1, 2}, rhs_t{3, 4}));
        }
    }
    GIVEN("vector<int> <-> vector<float>")
    {
        WHEN("assigning")
        {
            AND_WHEN("vector<int> -> vector<float>")
            {
                using dest_t = std::vector<float>;
                using source_t = std::vector<int>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
                TEST_MOVE_ASSIGNMENT<dest_t, source_t>({1, 2, 3});
            }
            AND_WHEN("vector<float> <- vector<int>")
            {
                using dest_t = std::vector<int>;
                using source_t = std::vector<float>;
                TEST_COPY_ASSIGNMENT<dest_t, source_t>({ 1.1f, 2.2f, 3.3f });
                TEST_MOVE_ASSIGNMENT<dest_t, source_t>({ 1.1f, 2.2f, 3.3f });
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("vector<int> == vector<float>")
            {
                using lhs_t = std::vector<int>;
                using rhs_t = std::vector<float>;
                REQUIRE(convertible::values::equal(lhs_t{1, 2}, rhs_t{1.0f, 2.0f}));
                REQUIRE(convertible::values::equal(lhs_t{1, 2}, rhs_t{1.4f, 2.6f}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1, 2}, rhs_t{2.0f, 2.0f}));
            }
            AND_WHEN("vector<float> == vector<int>")
            {
                using lhs_t = std::vector<float>;
                using rhs_t = std::vector<int>;
                REQUIRE(convertible::values::equal(lhs_t{1.0f, 2.0f}, rhs_t{1, 2}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{1.4f, 2.6f}, rhs_t{1, 2}));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{2.0f, 2.0f}, rhs_t{1, 2}));
            }
        }
    }
}

SCENARIO("convertible_tests: Assignment & Equality with converter")
{
    GIVEN("int <-> string")
    {
        WHEN("assigning")
        {
            AND_WHEN("int -> string")
            {
                using dest_t = std::string;
                using source_t = int;
                TEST_COPY_ASSIGNMENT<dest_t>(source_t{12}, int_string_converter);
            }
            AND_WHEN("int <- string")
            {
                using dest_t = int;
                using source_t = std::string;
                TEST_COPY_ASSIGNMENT<dest_t>(source_t{"12"}, int_string_converter);
                TEST_MOVE_ASSIGNMENT<dest_t>(source_t{"12"}, int_string_converter);
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("int == string")
            {
                using lhs_t = int;
                using rhs_t = std::string;
                REQUIRE(convertible::values::equal(lhs_t{12}, rhs_t{"12"}, int_string_converter));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{12}, rhs_t{"34"}, int_string_converter));
            }
            AND_WHEN("string == int")
            {
                using lhs_t = std::string;
                using rhs_t = int;
                REQUIRE(convertible::values::equal(lhs_t{"12"}, rhs_t{12}, int_string_converter));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{"12"}, rhs_t{34}, int_string_converter));
            }
        }
    }
    GIVEN("vector<int> <-> vector<string>")
    {
        WHEN("assigning")
        {
            AND_WHEN("vector<int> -> vector<string>")
            {
                using dest_t = std::vector<std::string>;
                using source_t = std::vector<int>;
                TEST_COPY_ASSIGNMENT<dest_t>(source_t{12, 13}, int_string_converter);
            }
            AND_WHEN("vector<int> <- vector<string>")
            {
                using dest_t = std::vector<int>;
                using source_t = std::vector<std::string>;
                TEST_COPY_ASSIGNMENT<dest_t>(source_t{"12", "13"}, int_string_converter);
                TEST_MOVE_ASSIGNMENT<dest_t>(source_t{"12", "13"}, int_string_converter);
            }
        }
        WHEN("comparing")
        {
            AND_WHEN("vector<int> == vector<string>")
            {
                using lhs_t = std::vector<int>;
                using rhs_t = std::vector<std::string>;
                REQUIRE(convertible::values::equal(lhs_t{12, 13}, rhs_t{"12", "13"}, int_string_converter));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{12, 32}, rhs_t{"12", "13"}, int_string_converter));
            }
            AND_WHEN("vector<string> == vector<int>")
            {
                using lhs_t = std::vector<std::string>;
                using rhs_t = std::vector<int>;
                REQUIRE(convertible::values::equal(lhs_t{"12", "13"}, rhs_t{12, 13}, int_string_converter));
                REQUIRE_FALSE(convertible::values::equal(lhs_t{"12", "32"}, rhs_t{12, 13}, int_string_converter));
            }
        }
    }
}

SCENARIO("convertible_tests: Member binding")
{
    struct type_a
    {
        int member;
    };

    GIVEN("a binding to a member")
    {
        constexpr auto member = convertible::binding::class_member(&type_a::member);
        THEN("value can be read")
        {
            type_a instance;
            instance.member = 10;
            REQUIRE(member.read(instance) == 10);
        }
    }
}