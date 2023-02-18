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
The result is a solution that is equally fast (or even marginally faster) given optimization is turned on:
```
$ bdep test config.cxx.coptions="-O3 -DNDEBUG"

| relative |               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | conversion
|---------:|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:-----------
|   100.0% |            7,129.69 |          140,258.56 |    0.1% |      196,571.09 |       38,504.24 |  5.105 |      38,006.02 |    0.0% |      0.01 | `convertible`
|    96.1% |            7,415.42 |          134,854.09 |    0.1% |      203,558.09 |       39,835.78 |  5.110 |      38,005.02 |    0.0% |      0.01 | `manual`

| relative |               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | equality
|---------:|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:---------
|   100.0% |            8,894.39 |          112,430.40 |    0.1% |      189,198.10 |       48,028.87 |  3.939 |      36,013.02 |    0.0% |      0.01 | `convertible`
|    98.6% |            9,018.78 |          110,879.75 |    0.0% |      196,586.11 |       48,923.54 |  4.018 |      36,011.03 |    0.0% |      0.01 | `manual`
```
_Tested on Ryzen 7950X with GCC 12.2.1._
