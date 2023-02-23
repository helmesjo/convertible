#include <convertible/convertible.hxx>
#include <nanobench.h>
#include <doctest/doctest.h>

#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

using namespace convertible;
namespace bench = ankerl::nanobench;

namespace
{
  auto gen_random_int()
  {
    return std::rand();
  }

  auto gen_random_str(std::size_t len)
  {
    static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (std::size_t i = 0; i < len; ++i)
    {
      tmp_s += alphanum[gen_random_int() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
  }

  struct type_a
  {
    int val1;
    std::string val2;
    std::vector<std::string> val3;
    std::optional<int> val4;
  };

  struct type_b
  {
    int val1;
    std::string val2;
    std::vector<int> val3;
    int val4;
  };

  struct int_string_converter
  {
    int operator()(const std::string& s) const
    {
      return std::stoi(s);
    }

    std::string operator()(int i) const
    {
      return std::to_string(i);
    }
  };

  auto create_type_a()
  {
    constexpr std::size_t size = 1000;

    type_a obj
    {
      gen_random_int(), 
        gen_random_str(size),
        {},
        999
    };

    obj.val3.resize(size);
    for(std::size_t i = 0; i < size; ++i)
    {
      obj.val3[i] = std::to_string(gen_random_int());
    }

    return obj;
  }

  auto create_type_b()
  {
    constexpr std::size_t size = 1000;

    type_b obj
    {
      gen_random_int(), 
        gen_random_str(size),
        {},
        0
    };

    obj.val3.resize(size);
    for(std::size_t i = 0; i < size; ++i)
    {
      obj.val3[i] = gen_random_int();
    }

    return obj;
  }
}

TEST_CASE("mapping_table")
{
  auto table = 
    mapping_table
    {
      mapping( member(&type_a::val1), member(&type_b::val1) ),
      mapping( member(&type_a::val2), member(&type_b::val2) ),
      mapping( member(&type_a::val3), member(&type_b::val3), int_string_converter{} ),
      mapping( deref(member(&type_a::val4)), member(&type_b::val4) )
    };

  auto lhs = create_type_a();
  auto rhs = create_type_b();

  bench::Bench b;
  b.warmup(500).relative(true);

  b.title("conversion")
    .run("convertible", [&] {
      table.assign<direction::rhs_to_lhs>(lhs, rhs);
      bench::doNotOptimizeAway(lhs);
      bench::doNotOptimizeAway(rhs);
  }).run("manual", [&] {
      lhs.val1 = rhs.val1;
      lhs.val2 = rhs.val2;
      lhs.val3.resize(rhs.val3.size());
      for(std::size_t i = 0; i < rhs.val3.size(); ++i)
      {
        lhs.val3[i] = int_string_converter{}(rhs.val3[i]);
      }
      lhs.val4 = rhs.val4;
      bench::doNotOptimizeAway(lhs);
      bench::doNotOptimizeAway(rhs);
  });
  b.title("equality")
    .run("convertible", [&] {
      bool equal;
      bench::doNotOptimizeAway(equal = table.equal(lhs, rhs));
  }).run("manual", [&] {
      bool equal = true;
      for(std::size_t i = 0; i < rhs.val3.size(); ++i)
      {
        if(lhs.val3[i] != int_string_converter{}(rhs.val3[i]))
        {
          equal = false;
          break;
        }
      }
      bench::doNotOptimizeAway(equal = equal && lhs.val1 == rhs.val1 && lhs.val2 == rhs.val2);
  });
}
