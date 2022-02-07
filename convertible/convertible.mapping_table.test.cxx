#include <convertible/convertible.hxx>
#include <convertible/doctest_include.hxx>

#include <memory>
#include <string>

SCENARIO("convertible: Mapping table")
{
  using namespace convertible;

  struct type_a
  {
    int val1;
    std::string val2;
  };

  struct type_b
  {
    int val1;
    std::string val2;
  };

  struct type_c
  {
    int val1;
  };

  THEN("it's constexpr constructible")
  {
    constexpr mapping_table table{
      mapping( object(), object() )
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
    struct type_c
    {
      int val1;
    };

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

    struct a_b_converter
    {
      a_b_converter():
        ref(*this)
      {}
      a_b_converter& ref;
      type_a operator()(type_b obj) const
      {
        if(obj.node)
        {
          return { obj.val, std::make_shared<type_a>(ref(*obj.node)) };
        }
        else
        {
          return { obj.val, nullptr };
        }
      }

      type_b operator()(type_a obj) const
      {
        if(obj.node)
        {
          return { obj.val, std::make_shared<type_b>(ref(*obj.node)) };
        }
        else
        {
          return { obj.val, nullptr };
        }
      }
    };

    mapping_table table{
      mapping( member(&type_a::val), member(&type_b::val) ),
      mapping( deref(member(&type_a::node)), deref(member(&type_b::node)), a_b_converter{} )
    };

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
        REQUIRE(lhs.node != nullptr);
        REQUIRE(lhs.node->val == rhs.node->val);
        REQUIRE(lhs.node->node == nullptr);
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
        REQUIRE(lhs.node != nullptr);
        REQUIRE(lhs.node->val != rhs.node->val);
        REQUIRE(lhs.node->node == nullptr);
        REQUIRE_FALSE(table.equal(lhs, rhs));
      }
    }
  }
}
