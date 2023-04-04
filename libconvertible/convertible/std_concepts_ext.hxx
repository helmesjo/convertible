#pragma once

#include <concepts>
#include <iterator>
#include <span>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace std_ext
{
  template<typename obj_t>
  concept trivially_copyable = std::is_trivially_copyable_v<std::remove_reference_t<obj_t>>;

  template<typename from_t, typename to_t>
  concept castable_to = requires
  {
    static_cast<to_t>(std::declval<from_t>());
  };

  template<typename T>
  concept member_ptr = std::is_member_pointer_v<T>;

  // Credit: https://en.cppreference.com/w/cpp/ranges/range
  template<typename T>
  concept range = requires(T& t)
  {
    std::begin(t);
    std::end  (t);
  };

  template<typename T, typename index_t = std::size_t>
  concept indexable = requires(T& t)
  {
    t[std::declval<index_t>()];
  };

  template<typename T>
  concept dereferencable = requires(T t)
  {
    *t;
    requires (!std::same_as<void, decltype(*t)>);
  };

  // containers/ranges

  template<typename cont_t>
  concept fixed_size_container = std::is_array_v<std::remove_reference_t<cont_t>> || (range<cont_t> && requires (cont_t c)
  {
    requires (decltype(std::span{ c })::extent != std::dynamic_extent);
  });

  // Very rudimental concept based on "Member Function Table" here: https://en.cppreference.com/w/cpp/container
  template<typename cont_t>
  concept sequence_container = range<cont_t>
    && requires(cont_t c){ { c.size() }; }
    && (requires(cont_t c){ { c.data() }; } || requires(cont_t c){ { c.resize(0) }; });

  // Very rudimental concept based on "Member Function Table" here: https://en.cppreference.com/w/cpp/container
  template<typename cont_t>
  concept associative_container = range<cont_t> && requires(cont_t container)
  {
    typename std::remove_cvref_t<cont_t>::key_type;
  };

  template<typename cont_t>
  concept mapping_container = associative_container<cont_t> && requires
  {
    typename std::remove_cvref_t<cont_t>::mapped_type;
  };

  template<typename cont_t>
  concept resizable = requires(std::remove_reference_t<cont_t> container)
  {
    container.resize(std::size_t{0});
  };
}

#undef FWD
