#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <array>
#include <concepts>
#include <string>
#include <type_traits>

#if defined(_WIN32) && _MSC_VER < 1930 // < VS 2022 (17.0)
#define MSVC_ENUM_FIX(...) int
#else
#define MSVC_ENUM_FIX(...) __VA_ARGS__
#endif

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

    // adaptable:
    {
        struct type
        {
            type create(int){ return {}; }
            int create(float) { return {}; }
            void create(double) { }
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
}
