[![Cpp Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

# convertible
Create reusable two-way conversions to increase maintainability without compromising performance.

Builds with all major compilers (**gcc**, **clang**, **apple-clang**, **msvc**, **mingw**).

## TL;DR:
```c++
#include <convertible/convertible.hxx>
using namespace convertible;

struct type_a {
  int val;
  vector<int> vals;
};

struct type_b {
  string str;
  vector<string> strs;
};

struct int_string_converter {
  int    operator()(const string&);
  string operator()(int);
};

constinit auto table = mapping_table(
   mapping(member(&type_a::val),  member(&type_b::str),  int_string_converter{}),
   mapping(member(&type_a::vals), member(&type_b::strs), int_string_converter{})
);

int main() {
  auto a = type_a{1, {2, 3}};
  auto b = table(a); // or `table.assign<direction::lhs_to_rhs>(a, b)`
  // b.str  == "1"
  // b.strs == {"2", "3"};

  b = type_b{"4", {"5", "6"}};
  a = table(b);      // or `table.assign<direction::rhs_to_lhs>(a, b)`
  // a.val  == 4
  // a.vals == {5, 6};

  return table.equal(a, b); // Returns true 
}

```
See [tests](./convertible/convertible.mapping_table.test.cxx) for more advanced examples.

## Build & Run Tests
_Using [build2](https://build2.org/install.xhtml)_
```c
$ bdep init --config-create @gcc cc config.cxx=g++  // Init in new config
$ bdep update @gcc                                  // Build
$ bdep test @gcc                                    // Run tests
$ b install config.install.root=../out              // Install
```

## Performance
Great care has been taken to ensure there are no runtime penalties (compared to the equivalent "manual" conversion code). This is achieved simply by only relying on compile-time indirections (to figure out type compatibility, return types etc) & perfect forwarding.
The result is a solution that is equally fast given optimization is turned on:
```
$ bdep test config.cxx.coptions="-O3 -DNDEBUG" config.convertible.benchmark=true

-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
mapping_conversion      0.197 ms        0.197 ms         3518
manual_conversion       0.200 ms        0.201 ms         3413
```

