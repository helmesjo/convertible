#include <convertible/convertible_v2.hxx>

#include <doctest/doctest.h>

#include <iostream>
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

template<typename T>
struct detect;

namespace adapters
{
    struct any{};

    template<typename obj_t = any>
    struct object
    {
        auto create(auto&& obj) const
        {
            return object<decltype(obj)>{FWD(obj)};
        }

        object() = default;
        explicit object(auto&& obj): obj_(FWD(obj))
        {
        }

        operator decltype(auto)() const
        {
            return obj_;
        }

        auto operator=(auto&& val)
        {
            return obj_ = val;
        }

        auto operator==(const auto& val) const
        {
            return obj_ == val;
        }

        obj_t obj_;
    };

    template<typename member_ptr_t, typename instance_t = traits::member_class_t<member_ptr_t>, bool is_rval = std::is_rvalue_reference_v<instance_t>>
        requires std::is_member_pointer_v<member_ptr_t>
    struct member
    {
        using value_t = traits::member_value_t<member_ptr_t>;

        member_ptr_t ptr_;
        std::decay_t<instance_t>* inst_;

        auto create(auto&& obj) const
        {
            return member<member_ptr_t, decltype(obj)>(ptr_, FWD(obj));
        }

        explicit member(member_ptr_t ptr): ptr_(ptr){}
        explicit member(member_ptr_t ptr, auto&& inst): ptr_(ptr), inst_(&inst)
        {
        }

        operator decltype(auto)() const
        {
            if constexpr(is_rval)
            {
                std::cout << "Moved from\n";
                return std::move(inst_->*ptr_);
            }
            else
            {
                return inst_->*ptr_;
            }
        }

        auto operator=(auto&& val)
        {
            return inst_->*ptr_ = FWD(val);
        }

        auto operator==(const auto& val) const
        {
            return inst_->*ptr_ == val;
        }
    };
}

namespace operators
{
    struct assign
    {
        decltype(auto) exec(auto&& lhs, auto&& rhs)
        {
            return FWD(lhs) = FWD(rhs);
        }
    };

    struct compare
    {
        decltype(auto) exec(auto&& lhs, auto&& rhs)
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

enum class direction
{
    lhs_to_rhs,
    rhs_to_lhs
};

template<typename lhs_adapter_t, typename rhs_adapter_t>
struct mapping
{
    explicit mapping(lhs_adapter_t lhsAdapter, rhs_adapter_t rhsAdapter):
        lhsAdapter_(std::move(lhsAdapter)),
        rhsAdapter_(std::move(rhsAdapter))
    {}

    template<direction dir>
    auto assign(auto&& lhs, auto&& rhs)
    {
        auto op = operators::assign{};
        if constexpr(dir == direction::rhs_to_lhs)
            return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
        else
            return op.exec(rhsAdapter_.create(FWD(rhs)), lhsAdapter_.create(FWD(lhs)));
    }

    auto compare(auto&& lhs, auto&& rhs) const
    {
        constexpr auto op = operators::compare{};
        return op.exec(lhsAdapter_.create(FWD(lhs)), rhsAdapter_.create(FWD(rhs)));
    }

    std::decay_t<lhs_adapter_t> lhsAdapter_;
    std::decay_t<rhs_adapter_t> rhsAdapter_;
};

//static_assert(convertible::concepts::cpp20::adaptable<adapter, std::int32_t>, "SADSAD");

auto assign(auto&& lhsAdapter, auto&& lhs, auto&& rhsAdapter, auto&& rhs)
{
    operators::assign op1;
    return op1.exec(lhsAdapter.create(FWD(lhs)), rhsAdapter.create(FWD(rhs)));
}

auto compare(auto&& lhsAdapter, auto&& lhs, auto&& rhsAdapter, auto&& rhs)
{
    operators::compare op1;
    return op1.exec(lhsAdapter.create(FWD(lhs)), rhsAdapter.create(FWD(rhs)));
}

SCENARIO("playground1")
{
    std::uint32_t val1, val2 = 0;
    adapters::object adapter1;
    adapters::object adapter2;

    operators::assign op1;
    operators::compare op2;

    val1 = 1;
    val2 = 2;
    assign(adapter1, val1, adapter2, 2);
    REQUIRE(val1 == 2);
    REQUIRE(compare(adapter1, val1, adapter2, 2));

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
    assign(mbr1, obj1, mbr2, std::move(obj2));
    assign(mbr1, obj1, adapter2, 2);
    REQUIRE(obj1.val == obj2.val);
    REQUIRE(compare(mbr1, obj1, mbr2, std::move(obj2)));

    mapping m(mbr1, adapter2);

    m.assign<direction::rhs_to_lhs>(obj1, 5);
    REQUIRE(obj1.val == 5);
    m.assign<direction::lhs_to_rhs>(obj1, val1);
    REQUIRE(val1 == 5);
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
