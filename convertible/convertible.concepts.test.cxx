#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <array>
#include <concepts>
#include <string>
#include <type_traits>

#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define MSVC_ENUM_FIX(...) int
#else
#define MSVC_ENUM_FIX(...) __VA_ARGS__
#endif

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
}

SCENARIO("convertible: Traits")
{
    using namespace convertible;

    // member pointer
    struct type
    {
        int member;
    };

    static_assert(std::is_same_v<type, convertible::traits::member_class_t<decltype(&type::member)>>);
    static_assert(std::is_same_v<int, convertible::traits::member_value_t<decltype(&type::member)>>);

    // range_value
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>>>);
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&>>);
    static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&&>>);
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

    // class_type:
    {
        struct type
        {
            int member;
        };

        static_assert(concepts::class_type<type>);
        static_assert(concepts::class_type<int> == false);
    }

    // member_ptr:
    {
        struct type
        {
            int member;
        };

        static_assert(concepts::member_ptr<decltype(&type::member)>);
        static_assert(concepts::member_ptr<type> == false);
    }

    // indexable
    {
        static_assert(concepts::indexable<std::array<int, 1>>);
        static_assert(concepts::indexable<int*>);
        static_assert(concepts::indexable<int> == false);
    }

    // dereferencable
    {
        static_assert(concepts::dereferencable<int*>);
        static_assert(concepts::dereferencable<int> == false);
    }

    // adaptable:
    {
        struct type
        {
            type make(int){ return {}; }
            int make(float) { return {}; }
            void make(double) { }
        };

        static_assert(concepts::adaptable<int, type>);
        static_assert(concepts::adaptable<float, type> == false);
        static_assert(concepts::adaptable<double, type> == false);
    }

    // mappable:
    {
        static_assert(concepts::mappable<tests::mappable_type, int, int>);
        static_assert(concepts::mappable<tests::mappable_type, int, std::string> == false);
    }

    // executable
    {
        constexpr auto lhs_to_rhs = direction::lhs_to_rhs;
        static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, int&, int&, converter::identity>);
        static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, int&, std::string&, int_string_converter>);
        static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, std::vector<int>&, std::vector<std::string>&, int_string_converter>);
        static_assert(concepts::executable_with<lhs_to_rhs, operators::assign, std::vector<std::string>&&, std::vector<std::string>&, converter::identity>);

        constexpr auto rhs_to_lhs = direction::rhs_to_lhs;
        static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, int&, int&, converter::identity>);
        static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, int&, std::string&, int_string_converter>);
        static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, std::vector<int>&, std::vector<std::string>&, int_string_converter>);
        static_assert(concepts::executable_with<rhs_to_lhs, operators::assign, std::vector<std::string>&, std::vector<std::string>&&, converter::identity>);
    }

    // castable_to
    {
        static_assert(concepts::castable_to<int, int>);
        static_assert(concepts::castable_to<int, float>);
        static_assert(concepts::castable_to<float, int>);

        static_assert(!concepts::castable_to<std::string, int>);
        static_assert(!concepts::castable_to<int, std::string>);
    }
}
