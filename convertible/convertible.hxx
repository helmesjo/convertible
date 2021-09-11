#pragma once

#include <algorithm>
#include <type_traits>
#include <utility>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace convertible
{
    namespace details
    {
        struct nonesuch
        {
            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };
    }
    namespace traits
    {
        namespace detection
        {
            // Detection idiom

            template<typename... any_t>
            struct tester{};

            template <typename Default, typename AlwaysVoid,
            template<class...> typename Op, typename... Args>
            struct detector {
                using value_t = std::false_type;
                using type = Default;
            };

            template <typename Default, template<typename...> typename Op, typename... Args>
            struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
                using value_t = std::true_type;
                using type = Op<Args...>;
            };

            template <template<typename...> typename Op, typename... Args>
            using is_detected = typename detector<details::nonesuch, void, Op, Args...>::value_t;
        }

        template <template<typename...> typename Op, typename... Args>
        constexpr bool is_detected_v = detection::is_detected<Op, Args...>::value;

        template<typename to_t, typename from_t>
        using is_static_castable_t = decltype(static_cast<to_t>(std::declval<from_t>()));
        template<typename to_t, typename from_t>
        constexpr bool is_static_castable_v = is_detected_v<is_static_castable_t, to_t, from_t>;

        template<typename T>
        using is_iterable_t = detection::tester<decltype(std::begin(std::declval<T&>()) == std::end(std::declval<T&>()))>;
        template<typename T>
        constexpr bool is_iterable_v = is_detected_v<is_iterable_t, T>;

        template<typename T>
        using element_type_t = std::conditional_t<is_iterable_v<T>, decltype(*std::begin(std::declval<T&>())), details::nonesuch>;

        template<typename T>
        using iterator_t = 
            std::conditional_t<is_iterable_v<T>, 
                std::conditional_t<std::is_rvalue_reference_v<T>, 
                    decltype(std::move_iterator(std::declval<T>().begin())), 
                    decltype(std::declval<T>().cbegin())>, 
                details::nonesuch>;

        template<typename T>
        using is_dynamic_container_t = detection::tester<decltype(std::declval<T&>().resize(std::declval<std::size_t>()))>;
        template<typename T>
        constexpr bool is_dynamic_container_v = is_iterable_v<T> && is_detected_v<is_dynamic_container_t, T>;

        template<typename converter_t, typename... arg_ts>
        using converted_t = std::invoke_result_t<converter_t, arg_ts...>;
        template<typename converter_t, typename... arg_ts>
        constexpr bool is_convertible_v = is_detected_v<converted_t, converter_t, arg_ts...>;

        template<typename T>
        using is_dereferencable_t = decltype(*std::declval<T>());
        template<typename T>
        constexpr bool is_dereferencable_v = is_detected_v<is_dereferencable_t, T>;

        template<typename T>
        using dereferenced_t = std::conditional_t<is_dereferencable_v<T>, decltype(*std::declval<T>()), details::nonesuch>;
    }
    
    namespace concepts
    {
        template<typename...>
        struct tag{};

        template<typename... ts>
        using tag_t = tag<ts...>*;

        template<bool is_true, typename to_t, typename from_t>
        using static_castable = std::enable_if_t<traits::is_static_castable_v<to_t, from_t> == is_true, tag_t<to_t, from_t>>;
    }

    namespace details
    {
        template<typename to_t, typename from_t>
        struct static_cast_converter
        {
            using result_t = std::decay_t<to_t>;

            template<typename arg_t,
                concepts::static_castable<true, result_t, arg_t> = nullptr>
            inline constexpr decltype(auto) operator()(arg_t&& from) const
            {
                return static_cast<result_t>(FWD(from));
            }
        };

        template<typename lhs_t, typename rhs_t>
        constexpr auto try_make_by_element_converter()
        {
            if constexpr(traits::is_iterable_v<lhs_t> && traits::is_iterable_v<rhs_t>)
            {
                return static_cast_converter<traits::element_type_t<lhs_t>, traits::element_type_t<rhs_t>>{};
            }
            else
            {
                // Fall back to something that's not invocable with any argument (force SFINAE)
                return []{};
            }
        }

        template<typename lhs_t, typename rhs_t>
        using by_element_static_cast_converter_t = decltype(details::try_make_by_element_converter<lhs_t, rhs_t>());
    }

    namespace traits
    {        
        // The two core type traits for two types that can be "bounded":
        // 1. Can rhs_t be statically casted to `decay_t<lhs_t>`?
        template<typename lhs_t, typename rhs_t, typename converter_t>
        using is_assignable_t = detection::tester<decltype(std::declval<lhs_t>() = std::declval<converted_t<converter_t, rhs_t>>())>;
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>>
        constexpr bool is_assignable_v = is_detected_v<is_assignable_t, lhs_t, rhs_t, converter_t>;

        // 2. Is `lhs_t == rhs_t` defined?
        template<typename lhs_t, typename rhs_t, typename converter_t>
        using is_comparable_t = detection::tester<decltype(std::declval<lhs_t>() == std::declval<converted_t<converter_t, rhs_t>>())>;
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>>
        constexpr bool is_comparable_v = is_detected_v<is_comparable_t, lhs_t, rhs_t, converter_t>;

        namespace class_info
        {
            namespace details
            {
                template<typename M>
                struct member_ptr_meta
                {
                    template<typename R>
                    struct return_t
                    {
                        using type = R;
                    };

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
    }

    namespace concepts
    {
        namespace cpp20
        {
            template <typename lhs_t, typename rhs_t, typename converter_t>
            concept assignable = requires(lhs_t lhs, rhs_t rhs, converter_t converter)
            {
                {lhs = converter(rhs)};
            };
        }
        template<bool is_true, typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>>
        using assignable = std::enable_if_t<traits::is_assignable_v<lhs_t, rhs_t, converter_t> == is_true, tag_t<lhs_t, rhs_t>>;

        template<bool is_true, typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>>
        using comparable = std::enable_if_t<traits::is_comparable_v<lhs_t, rhs_t, converter_t> == is_true, tag_t<lhs_t, rhs_t>>;

        template<bool is_true, typename callable_t, typename... arg_ts>
        using invocable = std::enable_if_t<std::is_invocable_v<callable_t, arg_ts...> == is_true, tag_t<callable_t, arg_ts...>>;

        template<bool is_true, typename base_t, typename derived_t>
        using base_of = std::enable_if_t<std::is_base_of_v<base_t, std::decay_t<derived_t>> == is_true, tag_t<base_t, derived_t>>;

        template<bool is_true, typename member_ptr_t>
        using member_ptr = std::enable_if_t<std::is_member_pointer_v<member_ptr_t> == is_true, tag_t<member_ptr_t>>;
    }

    namespace values
    {
        /*!
        Direct assignment: `lhs = converter(rhs)`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>>
            requires concepts::cpp20::assignable<lhs_t, rhs_t, converter_t>
        inline void assign(lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter = {})
        {
            FWD(lhs) = FWD(converter)(FWD(rhs));
        }

        /*!
        Assignment by dereference: `if(rhs){ lhs = converter(*rhs) }`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, traits::dereferenced_t<rhs_t>>,
            concepts::assignable<false, lhs_t, rhs_t, converter_t> = nullptr,
            concepts::assignable<true, lhs_t, traits::dereferenced_t<rhs_t>, converter_t> = nullptr>
        inline void assign(lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter = {})
        {
            if (rhs)
            {
                assign(FWD(lhs), *FWD(rhs), FWD(converter));
            }
        }

        /*!
        Assignment by dereference: `if(lhs){ *lhs = converter(rhs) }`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<traits::dereferenced_t<lhs_t>, rhs_t>,
            concepts::assignable<false, lhs_t, rhs_t, converter_t> = nullptr,
            concepts::assignable<true, traits::dereferenced_t<lhs_t>, rhs_t, converter_t> = nullptr>
        inline void assign(lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter = {})
        {
            if (lhs)
            {
                assign(*FWD(lhs), FWD(rhs), FWD(converter));
            }
        }

        /*!
        Direct comparison: `lhs == converter(rhs)`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, rhs_t>,
            concepts::comparable<true, lhs_t, rhs_t, converter_t> = nullptr>
        inline bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {})
        {
            return lhs == FWD(converter)(rhs);
        }

        /*!
        Comparison by dereference: `if(rhs){ lhs == converter(*rhs) }`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<lhs_t, traits::dereferenced_t<rhs_t>>,
            concepts::comparable<false, lhs_t, rhs_t, converter_t> = nullptr,
            concepts::comparable<true, lhs_t, traits::dereferenced_t<rhs_t>, converter_t> = nullptr>
        inline bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {})
        {
            return rhs && equal(FWD(lhs), *FWD(rhs), FWD(converter));
        }

        /*!
        Comparison by dereference: `if(lhs){ *lhs == converter(rhs) }`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::static_cast_converter<traits::dereferenced_t<lhs_t>, rhs_t>,
            concepts::comparable<false, lhs_t, rhs_t, converter_t> = nullptr,
            concepts::comparable<true, traits::dereferenced_t<lhs_t>, rhs_t, converter_t> = nullptr>
        inline bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {})
        {
            return lhs && equal(*FWD(lhs), FWD(rhs), FWD(converter));
        }

        /*!
        Element-wise assignment: `lhs[i] = converter(rhs[i])`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::by_element_static_cast_converter_t<lhs_t, rhs_t>,
            concepts::assignable<false, lhs_t, rhs_t> = nullptr,
            concepts::assignable<true, traits::element_type_t<lhs_t>, traits::element_type_t<rhs_t>, converter_t> = nullptr>
        inline void assign(lhs_t&& lhs, rhs_t&& rhs, converter_t&& converter = {})
        {
            if constexpr(traits::is_dynamic_container_v<lhs_t>)
            {
                lhs.resize(rhs.size());
            }

            using iter_t = traits::iterator_t<rhs_t>;
            auto rhsBegin = iter_t(rhs.begin());
            auto lhsBegin = lhs.begin();
            for(; rhsBegin != rhs.end(); ++rhsBegin, ++lhsBegin)
            {
                assign(*lhsBegin, *rhsBegin, FWD(converter));
            }

            if constexpr(traits::is_dynamic_container_v<rhs_t> && std::is_rvalue_reference_v<decltype(rhs)>)
            {
                rhs.clear();
            }
        }

        /*!
        Element-wise comparison: `lhs[i] == converter(rhs[i])`
        */
        template<typename lhs_t, typename rhs_t, typename converter_t = details::by_element_static_cast_converter_t<lhs_t, rhs_t>,
            concepts::comparable<false, lhs_t, rhs_t> = nullptr,
            concepts::comparable<true, traits::element_type_t<lhs_t>, traits::element_type_t<rhs_t>, converter_t> = nullptr>
        inline bool equal(const lhs_t& lhs, const rhs_t& rhs, converter_t&& converter = {})
        {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                [&converter](const auto& l, const auto& r){
                    return equal(l, r, FWD(converter));
                });
        }
    }

    namespace binding
    {
        namespace class_info = convertible::traits::class_info;

        template<typename member_ptr_t>
        struct class_member
        {
            using class_t = class_info::member_class_t<member_ptr_t>;

            constexpr class_member(member_ptr_t ptr) : ptr_(ptr){}

            template<typename instance_t,
                concepts::base_of<true, class_t, instance_t> = nullptr>
            constexpr decltype(auto) read(instance_t&& instance) const
            {
                return FWD(instance).*ptr_;
            }

        private:
            member_ptr_t ptr_;
        };

        template<typename member_ptr_t,
            concepts::member_ptr<true, member_ptr_t> = nullptr>
        class_member(member_ptr_t) -> class_member<member_ptr_t>;
    }
}

#undef FWD