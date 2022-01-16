#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <array>
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
        // reference
        if constexpr(std::is_rvalue_reference_v<obj_t>)
        {
            static_assert(std::is_rvalue_reference_v<out_t>);
        }
        else
        {
            static_assert(!std::is_rvalue_reference_v<out_t>);
        }
        if constexpr(std::is_lvalue_reference_v<obj_t>)
        {
            static_assert(std::is_lvalue_reference_v<out_t>);
        }

        // common_reference_with
        static_assert(std::common_reference_with<adapter_t, adapter_t>);
        static_assert(std::common_reference_with<adapter_t, out_t>);
        static_assert(std::common_reference_with<out_t, adapter_t>);

        // convertible_to
        static_assert(std::convertible_to<adapter_t, adapter_t>);
        static_assert(std::convertible_to<adapter_t, out_t>);
        static_assert(std::convertible_to<const adapter_t, out_t>);

        // castable_to
        {
            struct dummy{};
            static_assert(concepts::castable_to<adapter_t, out_t>);
            static_assert(concepts::castable_to<const adapter_t, value_t>);

            static_assert(!concepts::castable_to<adapter_t, dummy>);
        }

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
        if constexpr(concepts::range<out_t>)
        {
            static_assert(concepts::range<adapter_t>);

            // Make sure dereferenced iterator returns correct value category
            if constexpr(std::is_lvalue_reference_v<out_t>)
            {
                using begin_val_t = decltype(*std::declval<adapter_t>().begin());
                using end_val_t = decltype(*std::declval<adapter_t>().end());
                static_assert(std::is_lvalue_reference_v<begin_val_t>);
                static_assert(std::is_lvalue_reference_v<end_val_t>);
            }
            if constexpr(std::is_rvalue_reference_v<out_t>)
            {
                using begin_val_t = decltype(*std::declval<adapter_t>().begin());
                using end_val_t = decltype(*std::declval<adapter_t>().end());
                static_assert(std::is_rvalue_reference_v<begin_val_t>);
                static_assert(std::is_rvalue_reference_v<end_val_t>);
                if constexpr(!std::is_const_v<std::remove_reference_t<obj_t>>)
                {
                    static_assert(!std::is_const_v<std::remove_reference_t<begin_val_t>>);
                    static_assert(!std::is_const_v<std::remove_reference_t<end_val_t>>);
                }
            }
        }
        else
        {
            static_assert(!concepts::range<adapter_t>);
        }

        // resizable
        if constexpr(concepts::resizable<out_t>)
        {
            static_assert(concepts::resizable<adapter_t>);
        }
        else
        {
            static_assert(!concepts::resizable<adapter_t>);
        }

        // dereferencable
        if constexpr(concepts::dereferencable<out_t>)
        {
            static_assert(concepts::dereferencable<adapter_t>);
        }
        else
        {
            static_assert(!concepts::dereferencable<adapter_t>);
        }
    }
    THEN("it's adaptable to the expected type")
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
    THEN("it's adaptable to the expected type")
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
            adapter::object<int*>,
            adapter::object<const int*>,
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
            std::string val = adapter;
            REQUIRE(val == str);
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

        THEN("it's explicitly castable to valid type")
        {
            enum class enum_a{ val = 1 };
            enum class enum_b{ val = 1 };

            REQUIRE(static_cast<enum_b>(adapter::object(enum_a::val)) == enum_b::val);
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
            std::string val = adapter;
            REQUIRE(val == obj.str);
        }
        THEN("it 'moves from' r-value reference")
        {
            obj.str = "world";

            auto adapterRval = adapter::member(&type::str, std::move(obj));

            const std::string movedTo = adapterRval;
            REQUIRE(movedTo == "world");
            REQUIRE(obj.str == "");

            std::string fromStr = "hello";
            adapterRval = std::move(fromStr);
            REQUIRE(fromStr == "");
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
            std::string val = adapter;
            REQUIRE(val == values[0]);
        }
        THEN("it 'moves from' r-value reference")
        {
            values[0] = "world";

            auto adapterRval = adapter::index<0>(std::move(values));

            const std::string movedTo = adapterRval;
            REQUIRE(movedTo == "world");
            REQUIRE(values[0] == "");

            std::string fromStr = "hello";
            adapterRval = std::move(fromStr);
            REQUIRE(fromStr == "");
        }
        THEN("equality operator works")
        {
            values[0] = "world";
            REQUIRE(adapter == "world");
            REQUIRE(adapter != "hello");
        }
    }
    GIVEN("dereference adapter")
    {
        struct int_wrapper
        {
            int operator*() const
            {
                return {};
            }
        };
        
        auto str = std::string();
        auto ptr = &str;
        auto adapter = adapter::deref(ptr);

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_held_type, 
           adapter::object<int*, adapter::reader::deref>,
           adapter::object<int_wrapper, adapter::reader::deref>
        );

        TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_similar_adapter, 
           std::pair<
               adapter::object<int*, adapter::reader::deref>,
               adapter::object<int_wrapper, adapter::reader::deref>
           >
        );

        THEN("it's constexpr constructible")
        {
            constexpr auto constexprAdapter = adapter::deref();
            (void)constexprAdapter;
        }
        THEN("it implicitly assigns member value")
        {
            adapter = "hello";
            REQUIRE(str == "hello");
        }
        THEN("it implicitly converts to type")
        {
            adapter = "hello";
            std::string val = adapter;
            REQUIRE(val == "hello");
        }
        THEN("it 'moves from' r-value reference")
        {
            auto adapterRval = adapter::deref(std::move(&str));

            str = "hello";
            const std::string movedTo = adapterRval;
            REQUIRE(movedTo == "hello");
            REQUIRE(str == "");

            std::string fromStr = "hello";
            adapterRval = std::move(fromStr);
            REQUIRE(fromStr == "");
        }
        THEN("equality operator works")
        {
            str = "hello";
            REQUIRE(adapter == "hello");
            REQUIRE(adapter != "world");
        }
    }
}

SCENARIO("convertible: Adapter composition")
{
    using namespace convertible;

    TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_held_type,
        adapter::object<adapter::object<int&>&>,
        adapter::object<adapter::object<int*, adapter::reader::deref>&>
    );

    TEST_CASE_TEMPLATE_INVOKE(shares_traits_with_similar_adapter,
        std::pair<
            adapter::object<adapter::object<int&>&>,
            adapter::object<adapter::object<float&>&>
        >,
        std::pair<
            adapter::object<adapter::object<int&>&>,
            adapter::object<adapter::object<float*, adapter::reader::deref>&>
        >,
        std::pair<
            adapter::object<int&>,
            adapter::object<adapter::object<float*, adapter::reader::deref>&>
        >,
        std::pair<
            adapter::object<int*, adapter::reader::deref>,
            adapter::object<adapter::object<float*, adapter::reader::deref>&>
        >
    );

    struct type
    {
        std::string* val = nullptr;
    };

    std::string str;
    type obj{ &str };

    GIVEN("composed adapter")
    {
        auto adapter = adapter::deref(adapter::member(&type::val, obj));

        THEN("it's constexpr constructible")
        {
            constexpr auto constexprAdapter = adapter::deref(adapter::object(adapter::member(&type::val)));
            (void)constexprAdapter;
        }
        THEN("it implicitly assigns member value")
        {
            adapter = "hello";
            REQUIRE(str == "hello");
        }
        THEN("it implicitly converts to type")
        {
            adapter = "hello";
            std::string val = adapter;
            REQUIRE(val == "hello");
        }
        THEN("it 'moves from' r-value reference")
        {
            str = "hello";
            type obj{ &str };
            auto adapterRval = adapter::deref(adapter::member(&type::val, std::move(obj)));

            const std::string movedTo = adapterRval;
            REQUIRE(movedTo == "hello");
            REQUIRE(str == "");

            std::string fromStr = "hello";
            adapterRval = std::move(fromStr);
            REQUIRE(fromStr == "");
        }
        THEN("it can make an adapter")
        {
            std::string str2 = "hello";
            type obj2{ &str2 };
            auto adapterTemplate = adapter::deref(adapter::member(&type::val));
            auto composedAdapter = adapterTemplate.make(std::move(obj2));

            const auto movedTo = static_cast<std::string>(composedAdapter);
            REQUIRE(movedTo == "hello");
            REQUIRE(str == "");
        }
    }
}

SCENARIO("convertible: Adapters constexpr-ness")
{
    using namespace convertible;

    WHEN("object & adapter is constexpr")
    {
        static constexpr int intVal = 1;
        static constexpr float floatVal = 1.0f;

        constexpr auto intAdapter = adapter::object(intVal);
        constexpr auto floatAdapter = adapter::object(floatVal);

        // Exercise all overloads
        THEN("constexpr construction")
        {
            constexpr decltype(intAdapter) adapter2(intAdapter);
            constexpr adapter::object<const int> adapter3(intAdapter);
            constexpr adapter::object<const int&> adapter4(intAdapter);
            constexpr adapter::object<const int> adapter5(floatAdapter);

            (void)adapter2;
            (void)adapter3;
            (void)adapter4;
            (void)adapter5;
        }
        THEN("constexpr conversion")
        {
            constexpr int val1 = intAdapter;
            constexpr const int& val2 = intAdapter;
            constexpr float val3 = intAdapter;

            static_assert(val1 == 1);
            static_assert(val2 == 1);
            static_assert(val3 == 1.0f);
        }
        THEN("constexpr assign")
        {
            constexpr decltype(intAdapter) adapter2 = intAdapter;
            constexpr auto adapter3 = (adapter::object<int>(0) = adapter::object<int>(0)) = adapter::object<float>(0.0) = (adapter::object<float>(0.0) = adapter::object(intVal));

            (void)adapter2;
            (void)adapter3;
        }
        THEN("constexpr comparison")
        {
            static_assert(intVal == intAdapter);
            static_assert(intAdapter == intVal);
            static_assert(intAdapter == intAdapter);
            static_assert(floatAdapter == intAdapter);
        }
        THEN("constexpr operators")
        {
            // *
            {
                constexpr auto ptrAdapter = adapter::object(&intVal);

                static_assert(*ptrAdapter == intVal);
            }
            // &
            {
                static_assert(&intAdapter == &intVal);
            }
            // begin, end, size
            {
                static constexpr auto iterable = std::array<int, 1>{};
                constexpr auto iterableAdapter = adapter::object(iterable);

                static_assert(iterableAdapter.begin() == iterable.begin());
                static_assert(iterableAdapter.end() == iterable.end());
                static_assert(iterableAdapter.size() == iterable.size());
            }
        }
    }
}
