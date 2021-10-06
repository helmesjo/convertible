#include <convertible/convertible_v2.hxx>

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace traits
{
    namespace details
    {
        template<typename M>
        struct member_ptr_meta
        {
            template<typename R>
            struct return_t{ using type = R; };

            template <typename C, typename R>
            static return_t<C> get_class_type(R C::*);

            template <typename C, typename R>
            static return_t<R> get_value_type(R C::*);

            using class_t = typename decltype(get_class_type(std::declval<M>()))::type;
            using value_t = typename decltype(get_value_type(std::declval<M>()))::type;
        };
    }

    template<typename member_ptr_t>
    using member_class_t = typename details::member_ptr_meta<member_ptr_t>::class_t;
    template<typename member_ptr_t, typename class_t = member_class_t<member_ptr_t>, typename value_t = typename details::member_ptr_meta<member_ptr_t>::value_t>
    using member_value_t = std::conditional_t<std::is_rvalue_reference_v<class_t>, value_t&&, value_t>;
}

namespace adapters
{
    template<typename obj_t>
    struct object
    {
        obj_t& obj_;

        explicit object(obj_t& obj): obj_(obj){}

        operator auto() const
        {
            return obj_;
        }

        auto operator=(const object& val)
        {
            return *this = static_cast<obj_t>(val);
        }

        auto operator=(const obj_t& val)
        {
            return obj_ = val;
        }

        auto operator==(const object& val) const
        {
            return *this == static_cast<obj_t>(val);
        }

        auto operator==(const obj_t& val) const
        {
            return obj_ == val;
        }
    };

    template<typename instance_t, typename member_ptr_t>
    struct member
    {
        using value_t = traits::member_value_t<member_ptr_t>;

        instance_t& inst_;
        member_ptr_t ptr_;

        constexpr explicit member(instance_t& inst, member_ptr_t ptr): inst_(inst), ptr_(ptr){}

        operator auto() const
        {
            return FWD(inst_).*ptr_;
        }

        auto operator=(const member& val)
        {
            return *this = static_cast<value_t>(val);
        }

        template<typename arg_t>
            requires std::convertible_to<arg_t, value_t>
        auto operator=(const arg_t& val)
        {
            return FWD(inst_).*ptr_ = val;
        }

        auto operator==(const member& val) const
        {
            return *this == static_cast<value_t>(val);
        }

        template<typename arg_t>
            requires std::convertible_to<arg_t, value_t>
        auto operator==(const arg_t& val) const
        {
            return FWD(inst_).*ptr_ == val;
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
    adapters::object adapter1(val1);
    adapters::object adapter2(val2);

    operators::assign op1;
    operators::compare op2;

    op1.exec(adapter1, adapter2);
    op2.exec(adapter1, adapter2);
}

SCENARIO("playground2")
{
    std::uint32_t val1, val2 = 0;
    adapters::object adapter1(val1);
    adapters::object adapter2(val2);

    operators::assign op1;
    operators::compare op2;

    val2 = 5;
    REQUIRE_FALSE(op2.exec(adapter1, adapter2));
    REQUIRE(op1.exec(adapter1, adapter2) == 5);
    REQUIRE(op2.exec(adapter1, adapter2));
}

SCENARIO("playground3")
{
    struct dummy
    {
        int val = 0;
    } obj;
    adapters::member mbrAdptr(obj, &dummy::val);

    std::uint32_t val1 = 1;
    adapters::object objAdptr(val1);

    operators::compare comp;

    REQUIRE_FALSE(comp.exec(objAdptr, mbrAdptr));
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
