#include <convertible/convertible_v2.hxx>

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace adapters
{
    template<typename value_t>
    struct value
    {
        value_t& inst_;

        explicit value(value_t& inst): inst_(inst){}

        operator value_t() const
        {
            return inst_;
        }

        auto operator=(const value& val)
        {
            return *this = static_cast<value_t>(val);
        }

        auto operator=(const value_t& val)
        {
            return inst_ = val;
        }

        auto operator==(const value& val) const
        {
            return *this == static_cast<value_t>(val);
        }

        auto operator==(const value_t& val) const
        {
            return inst_ == val;
        }
    };
}

namespace operators
{
    struct assign
    {
        template<typename lhs_t, typename rhs_t>
        decltype(auto) exec(lhs_t&& lhs, rhs_t&& rhs)
        {
            return FWD(lhs) = FWD(rhs);
        }
    };

    struct compare
    {
        template<typename lhs_t, typename rhs_t>
        decltype(auto) exec(lhs_t&& lhs, rhs_t&& rhs)
        {
            return FWD(lhs) == FWD(rhs);
        }
    };
}

// enum class direction
// {
//     lhs_to_rhs,
//     rhs_to_lhs
// };

// template<typename lhs_adapter_t, typename rhs_adapter_t>
// struct mapping
// {
//     template<direction dir, typename lhs_t, typename rhs_t>
//         requires 
//             convertible::concepts::cpp20::mappable<lhs_adapter_t, lhs_t, rhs_adapter_t, rhs_t>
//     void assign(lhs_t&& lhs, rhs_t&& rhs)
//     {
//         lhs_adapter_t l(lhs);
//         rhs_adapter_t r(rhs);
//         if constexpr(dir == direction::lhs_to_rhs)
//         {
//             r = static_cast<std::decay_t<lhs_t>>(l);
//         }
//         else
//         {
//             l = static_cast<std::decay_t<rhs_t>>(r);
//         }
//     }

//     template<typename lhs_t, typename rhs_t>
//         requires 
//             convertible::concepts::cpp20::mappable<lhs_adapter_t, lhs_t, rhs_adapter_t, rhs_t>
//     bool equals(lhs_t&& lhs, rhs_t&& rhs)
//     {
//         return lhs_adapter_t(lhs) == static_cast<std::decay_t<rhs_t>>(rhs_adapter_t(rhs));
//     }
// };

//static_assert(convertible::concepts::cpp20::adaptable<adapter, std::int32_t>, "SADSAD");

SCENARIO("playground1")
{
    std::uint32_t val1, val2 = 0;
    adapters::value adapter1(val1);
    adapters::value adapter2(val2);

    operators::assign op1;
    operators::compare op2;

    op1.exec(adapter1, adapter2);
    op2.exec(adapter1, adapter2);
}

SCENARIO("playground2")
{
    std::uint32_t val1, val2 = 0;
    adapters::value adapter1(val1);
    adapters::value adapter2(val2);

    operators::assign op1;
    operators::compare op2;

    val2 = 5;
    REQUIRE_FALSE(op2.exec(adapter1, adapter2));
    REQUIRE(op1.exec(adapter1, adapter2) == 5);
    REQUIRE(op2.exec(adapter1, adapter2));
}

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
