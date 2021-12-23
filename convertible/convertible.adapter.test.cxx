#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <iostream> // Fix libc++ link error with doctest
#include <string>
#include <tuple>
#include <type_traits>

TEST_CASE_TEMPLATE_DEFINE("it shares traits with held type", adapter_t, shares_traits_with_held_type)
{
    using namespace convertible;

    static_assert(!std::is_reference_v<adapter_t>, "Don't pass in a reference to adapter");
    static_assert(concepts::adapter<adapter_t>);

    using obj_t = typename adapter_t::object_t;
    using out_t = typename adapter_t::out_t;
    using value_t = typename adapter_t::value_t;

    THEN("it shares traits with held type")
    {
        // common_reference_with
        static_assert(std::common_reference_with<adapter_t, adapter_t>);
        static_assert(std::common_reference_with<adapter_t, out_t>);
        static_assert(std::common_reference_with<out_t, adapter_t>);

        // convertible_to
        static_assert(std::convertible_to<adapter_t, adapter_t>);
        static_assert(std::convertible_to<adapter_t, out_t>);
        static_assert(std::convertible_to<const adapter_t, out_t>);

        // assignable_from
        static_assert(!std::assignable_from<const adapter_t&, out_t>);

        if constexpr(std::assignable_from<value_t&, value_t&>)
        {
            static_assert(std::assignable_from<adapter_t&, const adapter_t&>);
            static_assert(std::assignable_from<adapter_t&, out_t>);
            static_assert(std::assignable_from<adapter_t&, value_t>);
            static_assert(std::assignable_from<value_t&, adapter_t>);
        }
        else
        {
            static_assert(!std::assignable_from<adapter_t&, out_t>);
            static_assert(!std::assignable_from<adapter_t&, value_t>);
            static_assert(!std::assignable_from<value_t&, adapter_t>);
        }

        // equality_comparable_with
        static_assert(std::equality_comparable_with<adapter_t, adapter_t>);
        static_assert(std::equality_comparable_with<adapter_t, out_t>);
        static_assert(std::equality_comparable_with<out_t, adapter_t>);

        // range
        if constexpr(std::ranges::range<out_t>)
        {
            static_assert(std::ranges::range<adapter_t>);
        }

        // resizable
        if constexpr(concepts::resizable<out_t>)
        {
            static_assert(concepts::resizable<adapter_t>);
        }
    }
    THEN("it's adaptable only to the expected type")
    {
        static_assert(concepts::adaptable<obj_t, adapter_t>);
    }
}

TEST_CASE_TEMPLATE_DEFINE("it shares traits with similar adapter", adapter_pair_t, shares_traits_with_similar_adapter)
{
    using namespace convertible;

    using lhs_adapter_t = std::tuple_element_t<0, adapter_pair_t>;
    using rhs_adapter_t = std::tuple_element_t<1, adapter_pair_t>;

    static_assert(!std::is_reference_v<lhs_adapter_t>, "Don't pass in a reference to adapter");
    static_assert(!std::is_reference_v<rhs_adapter_t>, "Don't pass in a reference to adapter");
    static_assert(concepts::adapter<lhs_adapter_t>);
    static_assert(concepts::adapter<rhs_adapter_t>);

    using lhs_object_t = typename lhs_adapter_t::object_t;
    using rhs_object_t = typename rhs_adapter_t::object_t;

    using lhs_out_t = typename lhs_adapter_t::out_t;
    using rhs_out_t = typename rhs_adapter_t::out_t;

    using lhs_value_t = typename lhs_adapter_t::value_t;
    using rhs_value_t = typename rhs_adapter_t::value_t;

    THEN("it shares traits with similar adapter")
    {
        // common_reference
        static_assert(std::common_reference_with<lhs_adapter_t, rhs_adapter_t>);
        static_assert(std::common_reference_with<rhs_adapter_t, lhs_adapter_t>);

        // convertible_to
        if constexpr (std::convertible_to<lhs_out_t, rhs_value_t>)
        {
            static_assert(std::convertible_to<lhs_adapter_t, rhs_value_t>);
        }
        else
        {
            static_assert(!std::convertible_to<lhs_adapter_t, rhs_value_t>);
        }
        if constexpr (std::convertible_to<rhs_out_t, lhs_value_t>)
        {
            static_assert(std::convertible_to<rhs_adapter_t, lhs_value_t>);
        }
        else
        {
            static_assert(!std::convertible_to<rhs_adapter_t, lhs_value_t>);
        }

        // assignable_from
        static_assert(!std::assignable_from<const lhs_adapter_t&, rhs_adapter_t>);
        static_assert(!std::assignable_from<const rhs_adapter_t&, lhs_adapter_t>);

        if constexpr (std::assignable_from<lhs_value_t&, rhs_value_t&>)
        {
            static_assert(std::assignable_from<lhs_adapter_t&, const rhs_adapter_t&>);
            static_assert(std::assignable_from<lhs_adapter_t&, rhs_adapter_t&>);
            static_assert(std::assignable_from<lhs_adapter_t&, rhs_adapter_t&&>);
        }
        else
        {
            static_assert(!std::assignable_from<lhs_adapter_t&, const rhs_adapter_t&>);
            static_assert(!std::assignable_from<lhs_adapter_t&, rhs_adapter_t&>);
            static_assert(!std::assignable_from<lhs_adapter_t&, rhs_adapter_t&&>);
        }
        if constexpr (std::assignable_from<rhs_value_t&, lhs_value_t&>)
        {
            static_assert(std::assignable_from<rhs_adapter_t&, const lhs_adapter_t&>);
            static_assert(std::assignable_from<rhs_adapter_t&, lhs_adapter_t&>);
            static_assert(std::assignable_from<rhs_adapter_t&, lhs_adapter_t&&>);
        }
        else
        {
            static_assert(!std::assignable_from<rhs_adapter_t&, const lhs_adapter_t&>);
            static_assert(!std::assignable_from<rhs_adapter_t&, lhs_adapter_t&>);
            static_assert(!std::assignable_from<rhs_adapter_t&, lhs_adapter_t&&>);
        }

        // equality_comparable_with
        static_assert(std::equality_comparable_with<const lhs_adapter_t&, const rhs_adapter_t&>);
        static_assert(std::equality_comparable_with<lhs_adapter_t&, rhs_adapter_t&>);
        static_assert(std::equality_comparable_with<rhs_adapter_t&&, lhs_adapter_t&&>);
    }
    THEN("it's adaptable only to the expected type")
    {
        static_assert(concepts::adaptable<lhs_object_t, lhs_adapter_t>);
        static_assert(concepts::adaptable<rhs_object_t, rhs_adapter_t>);
    }
}

SCENARIO("convertible: Adapters")
{
    using namespace convertible;

    GIVEN("object adapter")
    {
        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_held_type, 
            adapter::object<int&>,
            adapter::object<const int&>,
            adapter::object<int&&>,
            adapter::object<const int&&>,
            adapter::object<std::string&>,
            adapter::object<std::string&&>,
            adapter::object<std::vector<int>&>,
            adapter::object<std::vector<int>&&>
        );

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_similar_adapter, 
           std::pair<
               adapter::object<int&>, 
               adapter::object<int&>
           >,
           std::pair<
               adapter::object<const int&>, 
               adapter::object<const int&>
           >,
           std::pair<
               adapter::object<int&&>, 
               adapter::object<int&&>
           >,
           std::pair<
               adapter::object<const int&&>, 
               adapter::object<const int&&>
           >,
           std::pair<
               adapter::object<const char*>,
               adapter::object<std::string&>
           >,
           std::pair<
               adapter::object<std::vector<int>&>,
               adapter::object<std::vector<int>&>
           >
        );

        std::string str = "hello";
        auto adapter = adapter::object(str);

        THEN("it's constexpr constructible")
        {
            constexpr auto constexprAdapter = adapter::object();
            (void)constexprAdapter;
        }
        THEN("it implicitly assigns value")
        {
            adapter = "world";
            REQUIRE(str == "world");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == str);
        }
        THEN("implicit conversion 'moves from' r-value reference")
        {
            adapter::object adapterRval(std::move(str));

            std::string assigned = adapterRval;
            REQUIRE(assigned == "hello");
            REQUIRE(str == "");
        }
        THEN("implicit conversion 'moves from' const r-value reference")
        {
            const adapter::object adapterRval(std::move(str));

            std::string assigned = adapterRval;
            REQUIRE(assigned == "hello");
            REQUIRE(str == "");
        }
        THEN("assign 'moves from' r-value reference")
        {
            std::string str2 = "hello";
            adapter = std::move(str2);
            REQUIRE(str2 == "");
        }
        THEN("equality operator works")
        {
            str = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
            REQUIRE("world" == adapter);
            REQUIRE("hello" != adapter);
            REQUIRE(adapter == adapter);
            REQUIRE_FALSE(adapter != adapter);
        }
    }
    GIVEN("member adapter")
    {
        struct type
        {
            std::string str;
        } obj;

        struct type_b
        {
            std::string str;
        } obj2;

        auto adapter = adapter::member(&type::str, obj);

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_held_type, 
           adapter::object<type&, adapter::reader::member<std::string type::*>>,
           adapter::object<type&&, adapter::reader::member<std::string type::*>>
        );

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_similar_adapter, 
           std::pair<
               adapter::object<type&, adapter::reader::member<std::string type::*>>,
               adapter::object<type_b&, adapter::reader::member<std::string type_b::*>>
           >
        );

        THEN("it's constexpr constructible")
        {
            constexpr auto constexprAdapter = adapter::member(&type::str);
            (void)constexprAdapter;
        }
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

            auto adapterRval = adapter::member(&type::str, std::move(obj));

            const auto movedTo = static_cast<std::string>(adapterRval);
            REQUIRE(movedTo == "world");

            // GCC/Clang/MSVC choose the conversion template with std::string_view instead
            // of std::string when doing static cast here, which causes it to not be moved-from.
            // See example here: https://godbolt.org/z/jPrhvMhWd
            //REQUIRE(obj.str == "");

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
    GIVEN("index adapter")
    {
        std::array values = {std::string("1")};
        auto adapter = adapter::index<0>(values);

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_held_type, 
           adapter::object<std::array<std::string, 1>&, adapter::reader::index<0>>,
           adapter::object<const std::array<std::string, 1>&, adapter::reader::index<0>>,
           adapter::object<std::array<std::string, 1>&&, adapter::reader::index<0>>,
           adapter::object<const std::array<std::string, 1>&&, adapter::reader::index<0>>
        );

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_similar_adapter, 
           std::pair<
               adapter::object<std::array<const char*, 1>&, adapter::reader::index<0>>,
               adapter::object<std::array<std::string, 1>&, adapter::reader::index<0>>
           >
        );

        THEN("it's constexpr constructible")
        {
            constexpr auto constexprAdapter = adapter::index<0>();
            (void)constexprAdapter;
        }
        THEN("it implicitly assigns member value")
        {
            adapter = "hello";
            REQUIRE(values[0] == "hello");
        }
        THEN("it implicitly converts to type")
        {
            REQUIRE(static_cast<std::string>(adapter) == values[0]);
        }
        THEN("it 'moves from' r-value reference")
        {
            values[0] = "world";

            auto adapterRval = adapter::index<0>(std::move(values));

            const auto movedTo = static_cast<std::string>(adapterRval);
            REQUIRE(movedTo == "world");

            // GCC/Clang/MSVC choose the conversion template with std::string_view instead
            // of std::string when doing static cast here, which causes it to not be moved-from.
            // See example here: https://godbolt.org/z/jPrhvMhWd
            //REQUIRE(values[0] == "");

            std::string str = "hello";
            adapterRval = std::move(str);
            REQUIRE(str == "");
        }
        THEN("equality operator works")
        {
            values[0] = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
}
