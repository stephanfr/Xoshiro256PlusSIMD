#include <catch2/catch_all.hpp>
#include <iostream>

#include "../include/Xoroshiro256Plus.h"
#include "../UnitTest/Xoroshiro256PlusReference.h"

typedef SEFUtility::RNG::Xoroshiro256Plus<SIMDInstructionSet::NONE> Xoroshiro256PlusSerial;

constexpr size_t NUM_ITERATIONS = 1000000;
constexpr uint64_t SEED = 1;



TEST_CASE("Benchmarks No AVX", "[basic]")
{
    BENCHMARK_ADVANCED("Reference")(Catch::Benchmark::Chronometer meter)
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoroshiro256PlusReference::s[0] = split_mix.next();
        Xoroshiro256PlusReference::s[1] = split_mix.next();
        Xoroshiro256PlusReference::s[2] = split_mix.next();
        Xoroshiro256PlusReference::s[3] = split_mix.next();

        uint64_t    sum = 0;

        meter.measure([&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += Xoroshiro256PlusReference::next();
            }
        });

        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        uint64_t    sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += rng.next();
            }
        });

        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial Bounded")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        uint64_t    sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += rng.next( 300, 900 );
            }
        });

        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial dnext()")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += rng.dnext();
            }
        });
        
        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial next4() no sums")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        meter.measure([&rng] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                rng.next4();
            }
        });
    };

    BENCHMARK_ADVANCED("Serial next4() sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoroshiro256PlusSerial::FourIntegerValues    next_values( rng.next4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial next4() bounded sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoroshiro256PlusSerial::FourIntegerValues    next_values( rng.next4( 200, 700 ) );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };
    
    BENCHMARK_ADVANCED("Serial dnext4() sum in double")(Catch::Benchmark::Chronometer meter)
    {
        Xoroshiro256PlusSerial rng(SEED);

        alignas(32) double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoroshiro256PlusSerial::FourDoubleValues    next_values( rng.dnext4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

}