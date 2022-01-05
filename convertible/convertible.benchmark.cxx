#include <benchmark/benchmark.h>
#include <convertible/convertible.hxx>

#include <cstdlib>
#include <string>
#include <vector>

using namespace convertible;

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
    };

    struct type_b
    {
        int val1;
        std::string val2;
        std::vector<int> val3;
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
            {}
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
        constexpr auto size = 1000;

        type_b obj
        {
            gen_random_int(), 
            gen_random_str(size),
            {}
        };

        obj.val3.resize(size);
        for(std::size_t i = 0; i < size; ++i)
        {
            obj.val3[i] = gen_random_int();
        }
        
        return obj;
    }
}

static void mapping_conversion(benchmark::State& state)
{
    constexpr auto table = 
        mapping_table
        {
            mapping( adapter::member(&type_a::val1), adapter::member(&type_b::val1) ),
            mapping( adapter::member(&type_a::val2), adapter::member(&type_b::val2) ),
            mapping( adapter::member(&type_a::val3), adapter::member(&type_b::val3), int_string_converter{} )
        };

    for (auto _ : state)
    {
        auto lhs = create_type_a();
        auto rhs = create_type_b();

        table.assign<direction::rhs_to_lhs>(lhs, rhs);

        bool equal = table.equal(lhs, rhs);

        benchmark::DoNotOptimize(equal);
        benchmark::DoNotOptimize(lhs);
        benchmark::DoNotOptimize(rhs);
    }
}

static void manual_conversion(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto lhs = create_type_a();
        auto rhs = create_type_b();

        lhs.val1 = rhs.val1;
        lhs.val2 = rhs.val2;
        lhs.val3.resize(rhs.val3.size());
        for(std::size_t i = 0; i < rhs.val3.size(); ++i)
        {
            lhs.val3[i] = int_string_converter{}(rhs.val3[i]);
        }

        bool equal = true;
        for(std::size_t i = 0; i < rhs.val3.size(); ++i)
        {
            if(lhs.val3[i] != int_string_converter{}(rhs.val3[i]))
            {
                equal = false;
                break;
            }
        }

        equal = equal &&
        lhs.val1 == rhs.val1 &&
        lhs.val2 == rhs.val2;

        benchmark::DoNotOptimize(equal);
        benchmark::DoNotOptimize(lhs);
        benchmark::DoNotOptimize(rhs);
    }
}

BENCHMARK(mapping_conversion)->Unit(benchmark::kMillisecond);
BENCHMARK(manual_conversion )->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
