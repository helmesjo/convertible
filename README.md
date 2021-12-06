[![Cpp Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

# convertible
...
## TL;DR:
```c++
#include <convertible/convertible.hxx>
...
```

## Build & Run Tests
_Using [build2](https://build2.org/install.xhtml)_
```c
> bdep init --config-create @gcc cc config.cxx=g++  // Init in a new configuration @gcc|clang|msvc
> b                                                 // Build
> b test                                            // Run tests
> b install config.install.root=../out              // Install
```
