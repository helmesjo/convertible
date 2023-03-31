#include <bitset>
#include <convertible/convertible.hxx>
#include <doctest/doctest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

SCENARIO("convertible: Mapping table")
{
  using namespace convertible;

  struct type_a
  {
    bool operator==(const type_a&) const = default;
    int val1;
    std::string val2;
  };

  struct type_b
  {
    bool operator==(const type_b&) const = default;
    int val1;
    std::string val2;
  };

  struct type_c
  {
    bool operator==(const type_c&) const = default;
    int val1;
  };

  THEN("it's constexpr constructible")
  {
    constexpr mapping_table table{
      mapping( adapter(), adapter() )
    };
    (void)table;
  }
  GIVEN("mapping table between \n\n\ta.val1 <-> b.val1\n\ta.val2 <-> b.val2\n")
  {
    mapping_table table{
      mapping( member(&type_a::val1), member(&type_b::val1) ),
      mapping( member(&type_a::val2), member(&type_b::val2) )
    };

    type_a lhs;
    type_b rhs;

    WHEN("assigning lhs to rhs")
    {
      lhs.val1 = 10;
      lhs.val2 = "hello";
      table.assign<direction::lhs_to_rhs>(lhs, rhs);

      THEN("lhs == rhs")
      {
        REQUIRE(lhs.val1 == rhs.val1);
        REQUIRE(lhs.val2 == rhs.val2);
        REQUIRE(table.equal(lhs, rhs));
      }
    }
    WHEN("assigning lhs (r-value) to rhs")
    {
      lhs.val1 = 10;
      lhs.val2 = "hello";
      table.assign<direction::lhs_to_rhs>(std::move(lhs), rhs);

      THEN("rhs is assigned")
      {
        REQUIRE(rhs.val1 == 10);
        REQUIRE(rhs.val2 == "hello");

        AND_THEN("lhs is moved from")
        {
          REQUIRE(lhs.val2 == "");
          REQUIRE_FALSE(table.equal(lhs, rhs));
        }
      }
    }
    WHEN("assigning rhs to lhs")
    {
      rhs.val1 = 10;
      rhs.val2 = "hello";
      table.assign<direction::rhs_to_lhs>(lhs, rhs);

      THEN("lhs == rhs")
      {
        REQUIRE(lhs.val1 == rhs.val1);
        REQUIRE(lhs.val2 == rhs.val2);
        REQUIRE(table.equal(lhs, rhs));
      }
    }
    WHEN("assigning rhs (r-value) to lhs")
    {
      rhs.val1 = 10;
      rhs.val2 = "hello";
      table.assign<direction::rhs_to_lhs>(lhs, std::move(rhs));

      THEN("lhs is assigned")
      {
        REQUIRE(lhs.val1 == 10);
        REQUIRE(lhs.val2 == "hello");
        REQUIRE(table.equal(lhs, type_b{10, "hello"}));

        AND_THEN("rhs is moved from")
        {
          REQUIRE(rhs.val2 == "");
          REQUIRE_FALSE(table.equal(lhs, rhs));
        }
      }
    }
  }

  GIVEN("mapping table between \n\n\ta.val1 <-> b.val1\n\ta.val1 <-> c.val1\n")
  {
    mapping_table table{
      mapping( member(&type_a::val1), member(&type_b::val1) ),
      mapping( member(&type_a::val1), member(&type_c::val1) )
    };

    type_a lhs_a;
    type_b rhs_b;
    type_c rhs_c;

    WHEN("assigning lhs (a) to rhs (b)")
    {
      lhs_a.val1 = 10;
      lhs_a.val2 = "hello";
      table.assign<direction::lhs_to_rhs>(lhs_a, rhs_b);

      THEN("a.val1 == b.val1")
      {
        REQUIRE(lhs_a.val1 == rhs_b.val1);
        REQUIRE(table.equal(lhs_a, rhs_b));
      }
    }
    WHEN("assigning lhs (a) to rhs (c)")
    {
      lhs_a.val1 = 10;
      lhs_a.val2 = "hello";
      table.assign<direction::lhs_to_rhs>(lhs_a, rhs_c);

      THEN("a.val1 == c.val1")
      {
        REQUIRE(lhs_a.val1 == rhs_c.val1);
        REQUIRE(table.equal(lhs_a, rhs_c));
      }
    }
  }
  GIVEN("mapping table with known lhs & rhs types")
  {
    type_a lhs_a;
    lhs_a.val1 = 3;
    lhs_a.val2 = "hello";
    type_b rhs_b;
    rhs_b.val1 = 6;
    rhs_b.val2 = "world";
    type_c rhs_c;
    rhs_c.val1 = 9;

    mapping_table table{
      mapping( member(&type_a::val1, lhs_a), member(&type_b::val1, rhs_b) ),
      mapping( member(&type_a::val1, lhs_a), member(&type_c::val1, rhs_c) )
    };
    using table_t = decltype(table);

    THEN("defaulted lhs type can be constructed")
    {
      auto copy = table.defaulted_lhs();
      static_assert(std::same_as<decltype(copy), typename table_t::lhs_unique_types>);
      auto copy_a = std::get<0>(copy);

      static_assert(std::same_as<decltype(copy_a), type_a>);
      INFO("lhs a:  ", lhs_a.val1, ", ", lhs_a.val2);
      INFO("copy a: ", copy_a.val1, ", ", copy_a.val2);
      REQUIRE(copy_a == lhs_a);
    }
    THEN("defaulted rhs type can be constructed")
    {
      auto copy = table.defaulted_rhs();
      static_assert(std::same_as<decltype(copy), typename table_t::rhs_unique_types>);
      auto copy_b = std::get<0>(copy);
      auto copy_c = std::get<1>(copy);

      static_assert(std::same_as<decltype(copy_b), type_b>);
      static_assert(std::same_as<decltype(copy_c), type_c>);
      INFO("rhs b:  ", rhs_b.val1, ", ", rhs_b.val2);
      INFO("copy b: ", copy_b.val1, ", ", copy_b.val2);
      REQUIRE(copy_b == rhs_b);
      INFO("rhs c:  ", rhs_c.val1);
      INFO("copy c: ", copy_c.val1);
      REQUIRE(copy_c == rhs_c);
    }
  }
}

SCENARIO("convertible: Mapping table constexpr-ness")
{
  using namespace convertible;

  WHEN("mapping table is constexpr")
  {
    struct type_a{ int val = 0; };
    struct type_b{ int val = 0; };
    static constexpr type_a lhsVal;
    static constexpr type_b rhsVal;

    constexpr auto lhsAdapter = member(&type_a::val);
    constexpr auto rhsAdapter = member(&type_b::val);

    constexpr auto table = mapping_table(mapping(lhsAdapter, rhsAdapter));

    THEN("constexpr construction")
    {
      constexpr decltype(table) table2(table);
      (void)table2;
    }
    THEN("constexpr conversion")
    {
      constexpr type_a a = table(type_b{5});
      constexpr type_b b = table(type_a{6});

      static_assert(a.val == 5);
      static_assert(b.val == 6);
    }
    THEN("constexpr comparison")
    {
      static_assert(table.equal(lhsVal, rhsVal));
    }
  }
}

SCENARIO("convertible: Mapping table as a converter")
{
  using namespace convertible;

  struct type_a
  {
    std::string val;
  };

  struct type_b
  {
    std::string val;
  };

  struct type_c
  {
    std::string val;
  };

  struct type_d
  {
    std::string val;
  };

  GIVEN("mapping table between \n\n\ta <-> b\n")
  {
    mapping_table table{
      mapping(member(&type_a::val), member(&type_b::val))
    };

    WHEN("invoked with a")
    {
      type_a a = { "hello" };
      type_b b = table(a);
      THEN("it returns b")
      {
        REQUIRE(b.val == a.val);
      }
    }
    WHEN("invoked with b")
    {
      type_b b = { "hello" };
      type_a a = table(b);
      THEN("it returns a")
      {
        REQUIRE(a.val == b.val);
      }
    }
  }
  GIVEN("mapping table between \n\n\ta <-> b\n\tc <-> d\n")
  {
    mapping_table table{
      mapping(member(&type_a::val), member(&type_b::val)),
      mapping(member(&type_c::val), member(&type_d::val))
    };

    WHEN("invoked with a")
    {
      type_a a = { "hello" };

      auto [b, d] = table(a);
      static_assert(std::same_as<decltype(b), type_b>);
      static_assert(std::same_as<decltype(d), type_d>);

      THEN("it returns b & d")
      {
        REQUIRE(b.val == a.val);
        REQUIRE(d.val == "");
      }
    }
    WHEN("invoked with b")
    {
      type_b b = { "hello" };

      auto [a, c] = table(b);
      static_assert(std::same_as<decltype(a), type_a>);
      static_assert(std::same_as<decltype(c), type_c>);

      THEN("it returns a & c")
      {
        REQUIRE(a.val == b.val);
        REQUIRE(c.val == "");
      }
    }
    WHEN("invoked with c")
    {
      type_c c = { "hello" };

      auto [b, d] = table(c);
      static_assert(std::same_as<decltype(b), type_b>);
      static_assert(std::same_as<decltype(d), type_d>);

      THEN("it returns b & d")
      {
        REQUIRE(b.val == "");
        REQUIRE(d.val == c.val);
      }
    }
  }
}

SCENARIO("convertible: Mapping table (misc use-cases)")
{
  using namespace convertible;

  GIVEN("Recursive:\n\n\ta.val <-> b.val\n\ta.node <-> b.node\n")
  {
    struct type_a
    {
      int val;
      std::shared_ptr<type_a> node;

      bool operator==(const type_a& obj) const
      {
        if(node && obj.node)
        {
          return val == obj.val && *node == *obj.node;
        }
        else
        {
          return val == obj.val && node == obj.node;
        }
      }
    };
    struct type_b
    {
      int val;
      std::shared_ptr<type_b> node;

      bool operator==(const type_b& obj) const
      {
        if(node && obj.node)
        {
          return val == obj.val && *node == *obj.node;
        }
        else
        {
          return val == obj.val && node == obj.node;
        }
      }
    };


    mapping_table tableBase{
      mapping( member(&type_a::val), member(&type_b::val) )
    };

    auto table = extend(tableBase,
      mapping( deref(member(&type_a::node)), deref(member(&type_b::node)), tableBase )
    );

    type_a lhs;
    type_b rhs;

    WHEN("assigning lhs to rhs")
    {
      lhs.val = 1;
      auto lhsNode = std::make_shared<type_a>(type_a{6, nullptr});
      lhs.node = lhsNode;
      rhs.val = 5;
      auto rhsNode = std::make_shared<type_b>(type_b{7, nullptr});
      rhs.node = rhsNode;
      table.assign<direction::rhs_to_lhs>(lhs, rhs);

      THEN("lhs == rhs")
      {
        REQUIRE(lhs.val == rhs.val);
        REQUIRE((lhs.node != nullptr));
        REQUIRE(lhs.node->val == rhs.node->val);
        REQUIRE((lhs.node->node == nullptr));
        REQUIRE(table.equal(lhs, rhs));
      }
    }
    WHEN("not assigning lhs to rhs")
    {
      lhs.val = 5;
      auto lhsNode = std::make_shared<type_a>(type_a{6, nullptr});
      lhs.node = lhsNode;
      rhs.val = lhs.val;
      auto rhsNode = std::make_shared<type_b>(type_b{7, nullptr});
      rhs.node = rhsNode;

      THEN("lhs != rhs")
      {
        REQUIRE(lhs.val == rhs.val);
        REQUIRE((lhs.node != nullptr));
        REQUIRE(lhs.node->val != rhs.node->val);
        REQUIRE((lhs.node->node == nullptr));
        REQUIRE_FALSE(table.equal(lhs, rhs));
      }
    }
  }
  GIVEN("binary serialization/deserialization")
  {
    struct type_a
    {
      std::int8_t int8_ = 0;
      std::int16_t int16_ = 0;
      std::int32_t int32_ = 0;
      std::int64_t int64_ = 0;
    };

    GIVEN("mapping between \n\n\ttype_b <- nested (type_a) -> binary\n")
    {
      struct type_b
      {
        std::int8_t int8_ = 0;
        type_a a = {};
      };

      mapping_table table_inner{
        mapping(member(&type_a::int8_),  binary<0,0>()),
        mapping(member(&type_a::int16_), binary<1,2>()),
        mapping(member(&type_a::int32_), binary<3,6>()),
        mapping(member(&type_a::int64_), binary<7,14>())
      };
      mapping_table table_outer{
        mapping(member(&type_b::int8_), binary<0,0>()),
        mapping(member(&type_b::a),     binary<0,0>(), table_inner)
      };

      auto lhs = type_b{};
      auto rhs = std::vector<std::byte>{};

      THEN("type_b can be serialized")
      {
        lhs.int8_ = 1;
        lhs.a.int8_ = 2;
        lhs.a.int16_ = 3;
        lhs.a.int32_ = 4;
        lhs.a.int64_ = 5;

        table_outer.assign<direction::lhs_to_rhs>(lhs, rhs);

        INFO("\ntype_b (serialized): ", rhs);
        REQUIRE(rhs.size() == 16);
        // type_b::int8_
        REQUIRE(rhs[0]  == std::byte{1});
        // type_a::int8_
        REQUIRE(rhs[1]  == std::byte{2});
        // type_a::int16_
        REQUIRE(rhs[2]  == std::byte{3}); // byte 1
        REQUIRE(rhs[3]  == std::byte{0}); // byte 2
        // type_a::int32_
        REQUIRE(rhs[4]  == std::byte{4}); // byte 1
        REQUIRE(rhs[5]  == std::byte{0}); // byte 2
        REQUIRE(rhs[6]  == std::byte{0}); // byte 3
        REQUIRE(rhs[7]  == std::byte{0}); // byte 4
        // type_a::int64_
        REQUIRE(rhs[8]  == std::byte{5}); // byte 1
        REQUIRE(rhs[9]  == std::byte{0}); // byte 2
        REQUIRE(rhs[10] == std::byte{0}); // byte 3
        REQUIRE(rhs[11] == std::byte{0}); // byte 4
        REQUIRE(rhs[12] == std::byte{0}); // byte 5
        REQUIRE(rhs[13] == std::byte{0}); // byte 6
        REQUIRE(rhs[14] == std::byte{0}); // byte 7
        REQUIRE(rhs[15] == std::byte{0}); // byte 8

        REQUIRE(table_outer.equal(lhs, rhs));
      }
      THEN("type_b can be de-serialized")
      {
        rhs.resize(16);
        // type_b::int8_
        rhs[0]  = std::byte{1};
        // type_a::int8_
        rhs[1]  = std::byte{2};
        // type_a::int16_
        rhs[2]  = std::byte{3}; // byte 1
        rhs[3]  = std::byte{0}; // byte 2
        // type_a::int32_
        rhs[4]  = std::byte{4}; // byte 1
        rhs[5]  = std::byte{0}; // byte 2
        rhs[6]  = std::byte{0}; // byte 3
        rhs[7]  = std::byte{0}; // byte 4
        // type_a::int64_
        rhs[8]  = std::byte{5}; // byte 1
        rhs[9]  = std::byte{0}; // byte 2
        rhs[10] = std::byte{0}; // byte 3
        rhs[11] = std::byte{0}; // byte 4
        rhs[12] = std::byte{0}; // byte 5
        rhs[13] = std::byte{0}; // byte 6
        rhs[14] = std::byte{0}; // byte 7
        rhs[15] = std::byte{0}; // byte 8

        INFO("\ntype_b (serialized): ", rhs);
        table_outer.assign<direction::rhs_to_lhs>(lhs, rhs);

        REQUIRE(lhs.int8_    == 1);
        REQUIRE(lhs.a.int8_  == 2);
        REQUIRE(lhs.a.int16_ == 3);
        REQUIRE(lhs.a.int32_ == 4);
        REQUIRE(lhs.a.int64_ == 5);
        REQUIRE(table_outer.equal(lhs, rhs));
      }
    }
  }
}

namespace std
{
  std::ostream& operator<<(std::ostream& os, const std::vector<std::byte>& value)
  {
    os << '|';
    for(std::byte byte : value)
    {
      os << std::bitset<8>(std::to_integer<int>(byte)) << '|';
    }
    return os;
  }
}