#pragma once

#include <convertible/std_concepts_compat.hxx>

#include <concepts>
#include <type_traits>

// std extended
namespace std_ext
{
  template<typename as_t, typename with_t>
  using like_t = decltype(std::forward_like<as_t>(std::declval<with_t>()));
}

namespace convertible
{
  
}