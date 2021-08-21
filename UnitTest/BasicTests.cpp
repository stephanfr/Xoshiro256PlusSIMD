#include <catch2/catch_all.hpp>
#include <iostream>

#include "../include/SIMDInstructionSet.h"

#include "../include/Xoshiro256Plus.h"
#include "Xoshiro256PlusReference.h"

typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::NONE> Xoshiro256PlusSerial;
typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::AVX2> Xoshiro256PlusAVX2;

constexpr size_t NUM_SAMPLES = 1000;
constexpr uint64_t SEED = 1;

TEST_CASE("Reference, Serial and SIMD Implementations Match", "[basic]")
{
    SECTION("Streams Match - next")
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoshiro256PlusReference::s[0] = split_mix.next();
        Xoshiro256PlusReference::s[1] = split_mix.next();
        Xoshiro256PlusReference::s[2] = split_mix.next();
        Xoshiro256PlusReference::s[3] = split_mix.next();

        Xoshiro256PlusSerial serial_rng(SEED);
        Xoshiro256PlusAVX2 avx2_rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            uint64_t next_ref = Xoshiro256PlusReference::next();
            uint64_t next_serial = serial_rng.next();

            REQUIRE(next_ref == next_serial);
        }
    }

    SECTION("Streams Match - next4")
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoshiro256PlusReference::s[0] = split_mix.next();
        Xoshiro256PlusReference::s[1] = split_mix.next();
        Xoshiro256PlusReference::s[2] = split_mix.next();
        Xoshiro256PlusReference::s[3] = split_mix.next();

        Xoshiro256PlusReference::long_jump();

        Xoshiro256PlusSerial serial_rng(SEED);
        Xoshiro256PlusAVX2 avx2_rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            uint64_t next_ref = Xoshiro256PlusReference::next();
            auto next_serial = serial_rng.next4();
            auto next_simd = avx2_rng.next4();

            REQUIRE(((next_ref == next_serial[0]) && (next_ref == next_simd[0])));
            REQUIRE(next_serial[1] == next_simd[1]);
            REQUIRE(next_serial[2] == next_simd[2]);
            REQUIRE(next_serial[3] == next_simd[3]);
        }
    }

    SECTION("Jump Matches")
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoshiro256PlusReference::s[0] = split_mix.next();
        Xoshiro256PlusReference::s[1] = split_mix.next();
        Xoshiro256PlusReference::s[2] = split_mix.next();
        Xoshiro256PlusReference::s[3] = split_mix.next();

        Xoshiro256PlusReference::jump();

        {
            Xoshiro256PlusSerial serial_rng(
                Xoshiro256PlusSerial(SEED),
                SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::NONE>::JumpOnCopy::Short);

            for (auto i = 0; i < NUM_SAMPLES; i++)
            {
                uint64_t next_ref = Xoshiro256PlusReference::next();
                uint64_t next_serial = serial_rng.next();

                REQUIRE(next_ref == next_serial);
            }
        }
    }

    SECTION("Long Jump Matches")
    {
        SEFUtility::RNG::SplitMix64 split_mix(SEED);

        Xoshiro256PlusReference::s[0] = split_mix.next();
        Xoshiro256PlusReference::s[1] = split_mix.next();
        Xoshiro256PlusReference::s[2] = split_mix.next();
        Xoshiro256PlusReference::s[3] = split_mix.next();

        Xoshiro256PlusReference::long_jump();

        {
            Xoshiro256PlusSerial serial_rng(
                Xoshiro256PlusSerial(SEED),
                SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::NONE>::JumpOnCopy::Long);

            for (auto i = 0; i < NUM_SAMPLES; i++)
            {
                uint64_t next_ref = Xoshiro256PlusReference::next();
                uint64_t next_serial = serial_rng.next();

                REQUIRE(next_ref == next_serial);
            }
        }
    }

    SECTION("Double Test")
    {
        Xoshiro256PlusSerial serial_rng(SEED);

        double mean = 0;

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            double next_serial = serial_rng.dnext();

            mean += next_serial;

            REQUIRE(((next_serial >= 0) && (next_serial <= 1)));
        }

        mean /= NUM_SAMPLES;

        REQUIRE(((mean > 0.48) && (mean < 0.52)));
    }

    SECTION("AVX Double Test")
    {
        Xoshiro256PlusAVX2 avx_rng(SEED);

        double mean = 0;

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next4_avx = avx_rng.dnext4();

            mean += next4_avx[0];
            mean += next4_avx[1];
            mean += next4_avx[2];
            mean += next4_avx[3];

            for (auto j = 0; j < 4; j++)
            {
                REQUIRE(((next4_avx[j] >= 0) && (next4_avx[j] <= 1)));
            }
        }

        mean /= (NUM_SAMPLES * 4);

        REQUIRE(((mean > 0.48) && (mean < 0.52)));
    }

    SECTION("Integer Bounding")
    {
        Xoshiro256PlusSerial rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_single = rng.next(100, 200);

            REQUIRE(((next_single >= 100) && (next_single < 200)));

            auto next_four = rng.next4( 200, 300 );

            REQUIRE(((next_four[0] >= 200) && (next_four[0] < 300)));
            REQUIRE(((next_four[1] >= 200) && (next_four[1] < 300)));
            REQUIRE(((next_four[2] >= 200) && (next_four[2] < 300)));
            REQUIRE(((next_four[3] >= 200) && (next_four[3] < 300)));
        }
    }

    SECTION("AVX Integer Bounding")
    {
        Xoshiro256PlusAVX2 rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_four = rng.next4( 200, 300 );

            REQUIRE(((next_four[0] >= 200) && (next_four[0] < 300)));
            REQUIRE(((next_four[1] >= 200) && (next_four[1] < 300)));
            REQUIRE(((next_four[2] >= 200) && (next_four[2] < 300)));
            REQUIRE(((next_four[3] >= 200) && (next_four[3] < 300)));
        }
    }

    SECTION("Serial and AVX Integer Bounding Match")
    {
        Xoshiro256PlusSerial serial_rng(SEED);
        Xoshiro256PlusAVX2 avx_rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_four_serial = serial_rng.next4( 200, 300 );
            auto next_four_avx = avx_rng.next4( 200, 300 );

            REQUIRE( next_four_serial[0] == next_four_avx[0] );
            REQUIRE( next_four_serial[1] == next_four_avx[1] );
            REQUIRE( next_four_serial[2] == next_four_avx[2] );
            REQUIRE( next_four_serial[3] == next_four_avx[3] );
        }
    }
    
    SECTION("Double Bounding")
    {
        Xoshiro256PlusSerial rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_single = rng.dnext(-300, 100);

            REQUIRE(((next_single > -300) && (next_single < 100)));

            auto next_four = rng.dnext4( 1, 3 );

            REQUIRE(((next_four[0] > 1) && (next_four[0] < 3)));
            REQUIRE(((next_four[1] > 1) && (next_four[1] < 3)));
            REQUIRE(((next_four[2] > 1) && (next_four[2] < 3)));
            REQUIRE(((next_four[3] > 1) && (next_four[3] < 3)));
        }
    }

    SECTION("AVX Double Bounding")
    {
        Xoshiro256PlusAVX2 rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_four = rng.dnext4( 1, 3 );

            REQUIRE(((next_four[0] > 1) && (next_four[0] < 3)));
            REQUIRE(((next_four[1] > 1) && (next_four[1] < 3)));
            REQUIRE(((next_four[2] > 1) && (next_four[2] < 3)));
            REQUIRE(((next_four[3] > 1) && (next_four[3] < 3)));
        }
    }

    SECTION("Serial and AVX Double Bounding Match")
    {
        Xoshiro256PlusSerial serial_rng(SEED);
        Xoshiro256PlusAVX2 avx_rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_four_serial = serial_rng.dnext4( 200, 300 );
            auto next_four_avx = avx_rng.dnext4( 200, 300 );

            REQUIRE( next_four_serial[0] == next_four_avx[0] );
            REQUIRE( next_four_serial[1] == next_four_avx[1] );
            REQUIRE( next_four_serial[2] == next_four_avx[2] );
            REQUIRE( next_four_serial[3] == next_four_avx[3] );
        }
    }
}
