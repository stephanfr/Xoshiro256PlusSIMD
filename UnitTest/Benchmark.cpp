#include <catch2/catch_all.hpp>
#include <iostream>

#include "../include/SIMDInstructionSet.h"

#include "../include/Xoshiro256Plus.h"
#include "Xoshiro256PlusReference.h"

typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::NONE> Xoshiro256PlusSerial;
typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::AVX2> Xoshiro256PlusAVX2;

constexpr size_t NUM_ITERATIONS = 1000000;
constexpr uint64_t SEED = 1;



TEST_CASE("Benchmarks", "[basic]")
{
    BENCHMARK_ADVANCED("Reference")(Catch::Benchmark::Chronometer meter)
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoshiro256PlusReference::s[0] = split_mix.next();
        Xoshiro256PlusReference::s[1] = split_mix.next();
        Xoshiro256PlusReference::s[2] = split_mix.next();
        Xoshiro256PlusReference::s[3] = split_mix.next();

        uint64_t    sum = 0;

        meter.measure([&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += Xoshiro256PlusReference::next();
            }
        });

        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial next()")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        uint64_t    sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                sum += rng.next();
            }
        });

        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("Serial next() Bounded")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

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
        Xoshiro256PlusSerial rng(SEED);

        meter.measure([&rng] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                rng.dnext();
            }
        });
    };

    BENCHMARK_ADVANCED("Serial dnext() bounded")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        meter.measure([&rng] {
            for (auto i = 0; i < NUM_ITERATIONS; i++)
            {
                rng.dnext( -33, 44 );
            }
        });

    };

    BENCHMARK_ADVANCED("Serial next4() no sums")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        meter.measure([&rng] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                rng.next4();
            }
        });
    };

#ifdef __AVX2_AVAILABLE__
    BENCHMARK_ADVANCED("Serial next4() sum in __m256i")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        __m256i  sum = _mm256_set1_epi64x( 0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_epi64( sum, rng.next4() );
            }
        });
        
        REQUIRE( sum[0] != 0 );
    };
#endif

    BENCHMARK_ADVANCED("Serial next4() sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusSerial::FourIntegerValues    next_values( rng.next4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

#ifdef __AVX2_AVAILABLE__
    BENCHMARK_ADVANCED("Serial next4() bounded sum in __m256i")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        __m256i  sum = _mm256_set1_epi64x( 0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_epi64( sum, rng.next4(300, 400) );
            }
        });
        
        REQUIRE( sum[0] != 0 );
    };
#endif

    BENCHMARK_ADVANCED("Serial next4() bounded sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusSerial::FourIntegerValues    next_values( rng.next4( 200, 700 ) );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

#ifdef __AVX2_AVAILABLE__
    BENCHMARK_ADVANCED("Serial dnext4() sum in __m256d")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        __m256d  sum = _mm256_set1_pd( 0.0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_pd( sum, rng.dnext4() );
            }
        });
        
        REQUIRE( sum[0] != 0.0 );
    };
#endif

    BENCHMARK_ADVANCED("Serial dnext4() sum in double")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        alignas(32) double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusSerial::FourDoubleValues    next_values( rng.dnext4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum != 0.0 );
    };

#ifdef __AVX2_AVAILABLE__
    BENCHMARK_ADVANCED("Serial dnext4() bounded sum in __m256d")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        __m256d  sum = _mm256_set1_pd( 0.0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_pd( sum, rng.dnext4( -100, 100 ) );
            }
        });
        
        REQUIRE( sum[0] != 0.0 );
    };
#endif

    BENCHMARK_ADVANCED("Serial dnext4() bounded sum in double")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusSerial rng(SEED);

        alignas(32) double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusSerial::FourDoubleValues    next_values( rng.dnext4(-100, 100) );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum != 0.0 );
    };

#ifdef __AVX2_AVAILABLE__
    BENCHMARK_ADVANCED("AVX next4() no sum")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        meter.measure([&rng] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                rng.next4();
            }
        });
    };

    BENCHMARK_ADVANCED("AVX next4() sum in _m256i")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        __m256i  sum = _mm256_set1_epi64x( 0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_epi64( sum, rng.next4() );
            }
        });
        
        REQUIRE( sum[0] != 0 );
    };

    BENCHMARK_ADVANCED("AVX next4() sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusAVX2::FourIntegerValues next_values( rng.next4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("AVX next4() bounded sum in _m256i")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        __m256i  sum = _mm256_set1_epi64x( 0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_epi64( sum, rng.next4( 300, 600 ) );
            }
        });
        
        REQUIRE( sum[0] != 0 );
    };

    BENCHMARK_ADVANCED("AVX next4() bounded sum in uint64_t")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        alignas(32) uint64_t  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                auto    next_values( rng.next4( 200, 700 ) );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum > 0 );
    };

    BENCHMARK_ADVANCED("AVX dnext4() sum in __m256d")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        __m256d  sum = _mm256_set1_pd( 0.0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_pd( sum, rng.dnext4() );
            }
        });
        
        REQUIRE( sum[0] != 0.0 );
    };

    BENCHMARK_ADVANCED("AVX dnext4() sum in double")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        alignas(32) double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                Xoshiro256PlusAVX2::FourDoubleValues next_values( rng.dnext4() );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum != 0.0 );
    };

    BENCHMARK_ADVANCED("AVX dnext4() bounded sum in __m256d")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        __m256d  sum = _mm256_set1_pd( 0.0 );

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                sum = _mm256_add_pd( sum, rng.dnext4( -100, 100 ) );
            }
        });
        
        REQUIRE( sum[0] != 0.0 );
    };

    BENCHMARK_ADVANCED("AVX dnext4() bounded sum in double")(Catch::Benchmark::Chronometer meter)
    {
        Xoshiro256PlusAVX2 rng(SEED);

        alignas(32) double  sum = 0;

        meter.measure([&rng,&sum] {
            for (auto i = 0; i < NUM_ITERATIONS / 4; i++)
            {
                auto    next_values( rng.dnext4(-100, 100) );

                sum += next_values[0];
                sum += next_values[1];
                sum += next_values[2];
                sum += next_values[3];
            }
        });
        
        REQUIRE( sum != 0.0 );
    };

    #endif
}