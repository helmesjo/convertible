#include <convertible/common.hxx>
#include <convertible/std_concepts_ext.hxx>

#include <array>
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace convertible;

/* TRAITS */

namespace member_meta
{
  struct type
  {
    int     member;
    double& fun(int, char*);
  };

  static_assert(std::is_same_v<type, traits::member_class_t<decltype(&type::member)>>);
  static_assert(std::is_same_v<int, traits::member_value_t<decltype(&type::member)>>);
  static_assert(std::is_same_v<type, traits::member_class_t<decltype(&type::fun)>>);
  static_assert(std::is_same_v<double&, traits::member_value_t<decltype(&type::fun)>>);
}

namespace as_container_t
{
  static_assert(
    std::is_same_v<std::vector<int>&, traits::as_container_t<std::vector<std::string>&, int>>);
  static_assert(std::is_same_v<std::vector<int> const&,
                               traits::as_container_t<std::vector<std::string> const&, int>>);
  static_assert(
    std::is_same_v<std::list<int>&, traits::as_container_t<std::list<std::string>&, int>>);
  static_assert(
    std::is_same_v<std::set<float>&, traits::as_container_t<std::set<std::string>&, float>>);
  static_assert(
    std::is_same_v<std::unordered_map<int, float>&,
                   traits::as_container_t<std::unordered_map<int, std::string>&, float>>);
}

/* CONCEPTS */

namespace member_ptr
{
  struct type
  {
    int     member;
    double& func(int, char*);
  };

  static_assert(concepts::member_ptr<decltype(&type::member)>);
  static_assert(concepts::member_ptr<decltype(&type::func)>);
  static_assert(!concepts::member_ptr<type>);
}

namespace indexable
{
  struct type
  {
    int operator[](std::size_t);
    int operator[](char const*);
  };

  static_assert(concepts::indexable<std::array<int, 1>&, decltype(details::const_value(1))>);
  static_assert(concepts::indexable<type, decltype(details::const_value("key"))>);
  static_assert(concepts::indexable<std::array<int, 1>>);
  static_assert(concepts::indexable<int*>);
  static_assert(!concepts::indexable<int>);
}

namespace dereferencable
{
  static_assert(concepts::dereferencable<int*>);
  static_assert(concepts::dereferencable<int> == false);
}

namespace range
{
  static_assert(concepts::range<decltype(std::declval<int[2]>())&>);
  static_assert(concepts::range<std::array<int, 0>&>);
  static_assert(concepts::range<std::vector<int>&>);
  static_assert(concepts::range<std::list<int>&>);
  static_assert(concepts::range<std::set<int>&>);
  static_assert(concepts::range<std::unordered_map<int, int>&>);
}

namespace castable_to
{
  static_assert(concepts::castable_to<int, int>);
  static_assert(concepts::castable_to<int, float>);
  static_assert(concepts::castable_to<float, int>);

  static_assert(!concepts::castable_to<std::string, int>);
  static_assert(!concepts::castable_to<int, std::string>);
}

namespace fixed_size_container
{
  static_assert(concepts::fixed_size_container<std::array<int, 1> const&>);
  static_assert(concepts::fixed_size_container<int const[1]>);
  static_assert(!concepts::fixed_size_container<std::vector<int> const&>);
  static_assert(!concepts::fixed_size_container<std::string const&>);
  static_assert(!concepts::fixed_size_container<std::set<int> const&>);
}

namespace sequence_container
{
  static_assert(concepts::sequence_container<std::array<int, 0> const&>);
  static_assert(concepts::sequence_container<std::vector<int> const&>);
  static_assert(concepts::sequence_container<std::list<int> const&>);
  static_assert(!concepts::associative_container<std::array<int, 0> const&>);
  static_assert(!concepts::associative_container<std::vector<int> const&>);
  static_assert(!concepts::associative_container<std::list<int> const&>);
}

namespace associative_container
{
  static_assert(concepts::associative_container<std::set<int> const&>);
  static_assert(concepts::associative_container<std::unordered_map<int, int> const&>);
  static_assert(!concepts::sequence_container<std::set<int> const&>);
  static_assert(!concepts::sequence_container<std::unordered_map<int, int> const&>);
}

namespace mapping_container
{
  static_assert(concepts::mapping_container<std::unordered_map<int, int> const&>);
  static_assert(concepts::mapping_container<std::unordered_map<int, int> const&>);
  static_assert(!concepts::mapping_container<std::set<int> const&>);
}

namespace range_value_t
{
  static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>>>);
  static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&>>);
  static_assert(std::is_same_v<std::string, traits::range_value_t<std::vector<std::string>&&>>);
}

namespace range_value_forwarded_t
{
  static_assert(
    std::is_same_v<std::string&&, traits::range_value_forwarded_t<std::vector<std::string>>>);
  static_assert(
    std::is_same_v<std::string&, traits::range_value_forwarded_t<std::vector<std::string>&>>);
  static_assert(std::is_same_v<std::string const&&,
                               traits::range_value_forwarded_t<std::vector<std::string> const&&>>);
}

namespace mapped_value_t
{
  static_assert(std::is_same_v<std::string, traits::mapped_value_t<std::set<std::string>>>);
  static_assert(
    std::is_same_v<std::string, traits::mapped_value_t<std::unordered_map<int, std::string>>>);
}

namespace mapped_value_forwarded_t
{
  static_assert(
    std::is_same_v<std::string&&, traits::mapped_value_forwarded_t<std::set<std::string>>>);
  static_assert(
    std::is_same_v<std::string&, traits::mapped_value_forwarded_t<std::set<std::string>&>>);
  static_assert(std::is_same_v<std::string const&,
                               traits::mapped_value_forwarded_t<std::set<std::string> const&>>);
  static_assert(
    std::is_same_v<std::string&&,
                   traits::mapped_value_forwarded_t<std::unordered_map<int, std::string>>>);
  static_assert(
    std::is_same_v<std::string&,
                   traits::mapped_value_forwarded_t<std::unordered_map<int, std::string>&>>);
  static_assert(
    std::is_same_v<std::string const&,
                   traits::mapped_value_forwarded_t<std::unordered_map<int, std::string> const&>>);
}

namespace unique_ts
{
  static_assert(std::is_same_v<std::tuple<int, float>, traits::unique_ts<int, float, int, float>>);
  static_assert(std::is_same_v<std::tuple<float, int>, traits::unique_ts<int, float, float, int>>);
}

namespace unique_derived_ts
{
  struct base
  {};

  struct derived_a : base
  {};

  struct derived_b : derived_a
  {};

  struct derived_c : base
  {};

  static_assert(std::is_same_v<std::tuple<derived_a>, traits::unique_derived_ts<base, derived_a>>);
  static_assert(
    std::is_same_v<std::tuple<derived_b>, traits::unique_derived_ts<base, derived_a, derived_b>>);
  static_assert(std::is_same_v<std::tuple<derived_b, derived_c>,
                               traits::unique_derived_ts<base, derived_a, derived_b, derived_c>>);
}
