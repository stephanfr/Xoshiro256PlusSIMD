#pragma once

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256+ 1.0, our best and fastest generator for floating-point
   numbers. We suggest to use its upper bits for floating-point
   generation, as it is slightly faster than xoshiro256++/xoshiro256**. It
   passes all tests we are aware of except for the lowest three bits,
   which might fail linearity tests (and just those), so if low linear
   complexity is not considered an issue (as it is usually the case) it
   can be used to generate 64-bit outputs, too.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

/*
    Stephan Friedl
    Derived from Public Domain code
*/

/*
    A note on Xoshiro256Plus:

   The statistics on this RNG are very good but it you *need* something for crypto - you may want to look
   for a different RNG.  Aside from crypto - this RNG should be perfectly fine.

    Anything is better than the C Lib rand().
*/

#include <assert.h>
#include <immintrin.h>
#include <stdint.h>

#include <array>
#include <limits>

#include "SplitMix64.h"

namespace SEFUtility::RNG
{
    template <SIMDInstructionSet SIMD>
    class Xoshiro256Plus
    {
       public:
        class FourIntegerValues
        {
           public:
            FourIntegerValues& operator=(FourIntegerValues) = delete;
            FourIntegerValues& operator=(const FourIntegerValues&) = delete;
            FourIntegerValues& operator=(FourIntegerValues&&) = delete;

#ifdef __AVX2_AVAILABLE__
            operator __m256i() const { return result_packed_; }
#endif

            uint64_t operator[](size_t index) const { return result_packed_[index]; }

           private:
            alignas(32) __m256i result_packed_;

            FourIntegerValues(uint64_t value1, uint64_t value2, uint64_t value3, uint64_t value4)
            {
                if (SIMD >= SIMDInstructionSet::AVX2)
                {
                    result_packed_ = _mm256_set_epi64x(value4, value3, value2, value1);
                }
                else
                {
                    result_packed_[0] = value1;
                    result_packed_[1] = value2;
                    result_packed_[2] = value3;
                    result_packed_[3] = value4;
                }
            }

            FourIntegerValues(__m256i value) : result_packed_(std::move(value)) {}

            FourIntegerValues(FourIntegerValues&& value_to_copy)
                : result_packed_(std::move(value_to_copy.result_packed_))
            {
            }

            FourIntegerValues(FourIntegerValues& value_to_copy) = delete;
            FourIntegerValues(const FourIntegerValues& value_to_copy) = delete;

            friend class Xoshiro256Plus;
        };

        class FourDoubleValues
        {
           public:
            FourDoubleValues& operator=(FourDoubleValues) = delete;
            FourDoubleValues& operator=(const FourDoubleValues&) = delete;
            FourDoubleValues& operator=(FourDoubleValues&&) = delete;

#ifdef __AVX2_AVAILABLE__
            operator __m256d() const { return result_packed_; }
#endif

            double operator[](size_t index) const { return result_packed_[index]; }

           private:
            alignas(32) __m256d result_packed_;

#ifdef __AVX2_AVAILABLE__
            FourDoubleValues(__m256d value) : result_packed_(std::move(value)) {}
#else
            FourDoubleValues(__m256d& value) : result_packed_(std::move(value)) {}
#endif

            FourDoubleValues(FourDoubleValues&& value_to_copy) : result_packed_(std::move(value_to_copy.result_packed_))
            {
            }

            FourDoubleValues(FourDoubleValues& value_to_copy) = delete;
            FourDoubleValues(const FourDoubleValues& value_to_copy) = delete;

            friend class Xoshiro256Plus;
        };

        enum class JumpOnCopy : int32_t
        {
            None = 0,
            Short,
            Long
        };

        Xoshiro256Plus(const uint64_t seed)
        {
            static_assert(SIMD != SIMDInstructionSet::AVX, "AVX RNG is not supported - just use NONE");

#ifndef __AVX2_AVAILABLE__
            static_assert(SIMD == SIMDInstructionSet::NONE,
                          "Cannot have an AVX2 RNG if AVX2 extensions are not available");
#endif

            SplitMix64 split_mix(seed);

            serial_state_[0] = split_mix.next();
            serial_state_[1] = split_mix.next();
            serial_state_[2] = split_mix.next();
            serial_state_[3] = split_mix.next();

            serial_next4_state_[0] = long_jump(serial_state_);
            serial_next4_state_[1] = long_jump(serial_next4_state_[0]);
            serial_next4_state_[2] = long_jump(serial_next4_state_[1]);
            serial_next4_state_[3] = long_jump(serial_next4_state_[2]);

            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                simd_state_ = SIMDState(serial_next4_state_);
            }
        }

        Xoshiro256Plus(const std::array<uint64_t, 4> seed) : serial_state_(seed)
        {
            static_assert(SIMD != SIMDInstructionSet::AVX, "AVX RNG is not supported - just use NONE");

#ifndef __AVX2_AVAILABLE__
            static_assert(SIMD == SIMDInstructionSet::NONE,
                          "Cannot have an AVX2 RNG if AVX2 extensions are not available");
#endif

            serial_next4_state_[0] = long_jump(serial_state_);
            serial_next4_state_[1] = long_jump(serial_next4_state_[0]);
            serial_next4_state_[2] = long_jump(serial_next4_state_[1]);
            serial_next4_state_[3] = long_jump(serial_next4_state_[2]);

            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                simd_state_ = SIMDState(serial_next4_state_);
            }
        }

        Xoshiro256Plus(const Xoshiro256Plus<SIMD>& rng_to_copy, JumpOnCopy jump_dist = JumpOnCopy::Short)
            : serial_state_(rng_to_copy.serial_state_),
              serial_next4_state_(rng_to_copy.serial_next4_state_),
              simd_state_(rng_to_copy.simd_state_, jump_dist)
        {
            switch (jump_dist)
            {
                case JumpOnCopy::None:
                    break;

                case JumpOnCopy::Short:
                    serial_state_ = jump(serial_state_);
                    serial_next4_state_[0] = jump(serial_next4_state_[0]);
                    serial_next4_state_[1] = jump(serial_next4_state_[1]);
                    serial_next4_state_[2] = jump(serial_next4_state_[2]);
                    serial_next4_state_[3] = jump(serial_next4_state_[3]);
                    break;

                case JumpOnCopy::Long:
                    serial_state_ = long_jump(serial_state_);
                    serial_next4_state_[0] = long_jump(serial_next4_state_[0]);
                    serial_next4_state_[1] = long_jump(serial_next4_state_[1]);
                    serial_next4_state_[2] = long_jump(serial_next4_state_[2]);
                    serial_next4_state_[3] = long_jump(serial_next4_state_[3]);
                    break;
            }
        }

        //
        //  Single uint64 at a time
        //
        //  Bounding is in the range of [lower,upper) - i.e. lower included, upper not
        //

        uint64_t next(void) { return next_internal(serial_state_); }

        uint64_t next(uint32_t lower_bound, uint32_t upper_bound)
        {
            assert(upper_bound > lower_bound);

            return (((uint64_t)((uint32_t)next()) * (uint64_t)(upper_bound - lower_bound)) >> 32) +
                   (uint64_t)lower_bound;
        }

        //
        //  Four uint64s at a time
        //
        //  Bounding is in the range of [lower,upper) - i.e. lower included, upper not
        //

        FourIntegerValues next4()
        {
            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                return simd_next4_internal(simd_state_);
            }
            else
            {
                return FourIntegerValues(next_internal(serial_next4_state_[0]), next_internal(serial_next4_state_[1]),
                                         next_internal(serial_next4_state_[2]), next_internal(serial_next4_state_[3]));
            }
        }

        FourIntegerValues next4(uint32_t lower_bound, uint32_t upper_bound)
        {
            assert(upper_bound > lower_bound);

            uint64_t range = upper_bound - lower_bound;

            auto four_ints = next4();

            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                return _mm256_add_epi64(_mm256_srli_epi64(_mm256_mul_epu32(four_ints, _mm256_set1_epi64x(range)), 32),
                                        _mm256_set1_epi64x(lower_bound));
            }
            else
            {
                four_ints.result_packed_[0] =
                    (((uint64_t)((uint32_t)four_ints[0]) * range) >> 32) + (uint64_t)lower_bound;
                four_ints.result_packed_[1] =
                    (((uint64_t)((uint32_t)four_ints[1]) * range) >> 32) + (uint64_t)lower_bound;
                four_ints.result_packed_[2] =
                    (((uint64_t)((uint32_t)four_ints[2]) * range) >> 32) + (uint64_t)lower_bound;
                four_ints.result_packed_[3] =
                    (((uint64_t)((uint32_t)four_ints[3]) * range) >> 32) + (uint64_t)lower_bound;

                return four_ints;
            }
        }

        //
        //  Single double in range [0,1] for default or [lower, upper] when bounds applied
        //

        double dnext(void)
        {
            union
            {
                uint64_t int_value;
                double double_value;
            };

            int_value = (next() >> 12) | DOUBLE_MASK;

            return double_value - 1.0;
        }

        double dnext(double lower_bound, double upper_bound)
        {
            return (dnext() * (upper_bound - lower_bound)) + lower_bound;
        }

        //
        //  Four doubles at a time - same bounding as single double
        //

        FourDoubleValues dnext4()
        {
            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                union
                {
                    __m256i result_packed_int;
                    __m256d result_packed_double;
                };

                result_packed_int = _mm256_or_si256(DOUBLE_MASK_PACKED, _mm256_srli_epi64(next4(), 12));

                return _mm256_sub_pd(result_packed_double, ONE_PACKED_DOUBLE);
            }
            else
            {
                union
                {
                    uint64_t int_value;
                    double double_value;
                };

                __m256d packed_result;

                int_value = (next_internal(serial_next4_state_[0]) >> 12) | DOUBLE_MASK;
                packed_result[0] = double_value - 1.0;

                int_value = (next_internal(serial_next4_state_[1]) >> 12) | DOUBLE_MASK;
                packed_result[1] = double_value - 1.0;

                int_value = (next_internal(serial_next4_state_[2]) >> 12) | DOUBLE_MASK;
                packed_result[2] = double_value - 1.0;

                int_value = (next_internal(serial_next4_state_[3]) >> 12) | DOUBLE_MASK;
                packed_result[3] = double_value - 1.0;

                return packed_result;
            }
        }

        FourDoubleValues dnext4(double lower_bound, double upper_bound)
        {
            union
            {
                std::array<double, 4> upper_bounded_array;
                __m256d upper_bounded_packed_double;
            };

            if constexpr (SIMD >= SIMDInstructionSet::AVX2)
            {
                return _mm256_add_pd( _mm256_mul_pd( dnext4(), _mm256_set1_pd( upper_bound - lower_bound) ), _mm256_set1_pd(lower_bound));
            }
            else
            {
                auto    result = dnext4();

                result.result_packed_[0] = ( result.result_packed_[0] * ( upper_bound - lower_bound)) + lower_bound;
                result.result_packed_[1] = ( result.result_packed_[1] * ( upper_bound - lower_bound)) + lower_bound;
                result.result_packed_[2] = ( result.result_packed_[2] * ( upper_bound - lower_bound)) + lower_bound;
                result.result_packed_[3] = ( result.result_packed_[3] * ( upper_bound - lower_bound)) + lower_bound;
            
                return result;
            }
        }

        //
        //  Jump Functions
        //

        //  This is the jump function for the generator. It is equivalent
        //     to 2^128 calls to next(); it can be used to generate 2^128
        //     non-overlapping subsequences for parallel computations.

        static std::array<uint64_t, 4> jump(const std::array<uint64_t, 4>& initial_state)
        {
            static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa,
                                            0x39abdc4529b1661c};

            std::array<uint64_t, 4> local_state(initial_state);
            std::array<uint64_t, 4> temp({0, 0, 0, 0});

            for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
            {
                for (int b = 0; b < 64; b++)
                {
                    if (JUMP[i] & UINT64_C(1) << b)
                    {
                        temp[0] ^= local_state[0];
                        temp[1] ^= local_state[1];
                        temp[2] ^= local_state[2];
                        temp[3] ^= local_state[3];
                    }

                    next_internal(local_state);
                }
            }

            return temp;
        }

        //  This is the long-jump function for the generator. It is equivalent to
        //      2^192 calls to next(); it can be used to generate 2^64 starting points,
        //      from each of which jump() will generate 2^64 non-overlapping
        //      subsequences for parallel distributed computations.

        static std::array<uint64_t, 4> long_jump(const std::array<uint64_t, 4>& initial_state)
        {
            static const uint64_t LONG_JUMP[] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241,
                                                 0x39109bb02acbe635};

            std::array<uint64_t, 4> local_state(initial_state);
            std::array<uint64_t, 4> temp({0, 0, 0, 0});

            for (int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
            {
                for (int b = 0; b < 64; b++)
                {
                    if (LONG_JUMP[i] & UINT64_C(1) << b)
                    {
                        temp[0] ^= local_state[0];
                        temp[1] ^= local_state[1];
                        temp[2] ^= local_state[2];
                        temp[3] ^= local_state[3];
                    }

                    next_internal(local_state);
                }
            }

            return temp;
        }

       private:
        static constexpr uint64_t DOUBLE_MASK = UINT64_C(0x3FF) << 52;

        typedef std::array<uint64_t, 4> SerialState;

        alignas(32) SerialState serial_state_;

        alignas(32) std::array<SerialState, 4> serial_next4_state_;

        static inline constexpr __m256d cnstexpr_mm256_set1_pd(double value)
        {
            return (__m256d){value, value, value, value};
        };

        static inline constexpr __m256i cnstexpr_mm256_set1_epi64x(int64_t value)
        {
            return (__m256i)(__v4di){value, value, value, value};
        };

        static constexpr __m256i DOUBLE_MASK_PACKED = cnstexpr_mm256_set1_epi64x(DOUBLE_MASK);
        static constexpr __m256i ONE_PACKED_INT64 = cnstexpr_mm256_set1_epi64x(1);
        static constexpr __m256i ZERO_PACKED_INT64 = cnstexpr_mm256_set1_epi64x(0);
        static constexpr __m256d ONE_PACKED_DOUBLE = cnstexpr_mm256_set1_pd(1.0);

#ifdef __AVX2_AVAILABLE__

        class alignas(32) SIMDState
        {
           public:
            SIMDState() {}

            SIMDState(const SIMDState& state_to_copy, JumpOnCopy jump_dist = JumpOnCopy::None)
                : uint64_array_state_(state_to_copy.uint64_array_state_)
            {
                switch (jump_dist)
                {
                    case JumpOnCopy::None:
                        break;

                    case JumpOnCopy::Short:
                        uint64_array_state_[0] = jump(uint64_array_state_[0]);
                        uint64_array_state_[1] = jump(uint64_array_state_[1]);
                        uint64_array_state_[2] = jump(uint64_array_state_[2]);
                        uint64_array_state_[3] = jump(uint64_array_state_[3]);
                        break;

                    case JumpOnCopy::Long:
                        uint64_array_state_[0] = long_jump(uint64_array_state_[0]);
                        uint64_array_state_[1] = long_jump(uint64_array_state_[1]);
                        uint64_array_state_[2] = long_jump(uint64_array_state_[2]);
                        uint64_array_state_[3] = long_jump(uint64_array_state_[3]);
                        break;
                }
            }

            SIMDState(const std::array<SerialState, 4>& state) : SIMDState(state[0], state[1], state[2], state[3]) {}

            SIMDState(const std::array<uint64_t, 4>& seed1, const std::array<uint64_t, 4>& seed2,
                      const std::array<uint64_t, 4>& seed3, const std::array<uint64_t, 4>& seed4)
            {
                packed_state_[0][0] = seed1[0];
                packed_state_[1][0] = seed1[1];
                packed_state_[2][0] = seed1[2];
                packed_state_[3][0] = seed1[3];

                packed_state_[0][1] = seed2[0];
                packed_state_[1][1] = seed2[1];
                packed_state_[2][1] = seed2[2];
                packed_state_[3][1] = seed2[3];

                packed_state_[0][2] = seed3[0];
                packed_state_[1][2] = seed3[1];
                packed_state_[2][2] = seed3[2];
                packed_state_[3][2] = seed3[3];

                packed_state_[0][3] = seed4[0];
                packed_state_[1][3] = seed4[1];
                packed_state_[2][3] = seed4[2];
                packed_state_[3][3] = seed4[3];
            }

            const __m256i operator[](size_t index) const { return packed_state_[index]; }
            __m256i& operator[](size_t index) { return packed_state_[index]; }

           private:
            union
            {
                __m256i packed_state_[4];
                std::array<std::array<uint64_t, 4>, 4> uint64_array_state_;
            };
        };

        static FourIntegerValues simd_next4_internal(SIMDState& state)
        {
            FourIntegerValues result(_mm256_add_epi64(state[0], state[3]));

            const __m256i temp = _mm256_slli_epi64(state[1], 17);

            state[2] = _mm256_xor_si256(state[2], state[0]);
            state[3] = _mm256_xor_si256(state[3], state[1]);
            state[1] = _mm256_xor_si256(state[1], state[2]);
            state[0] = _mm256_xor_si256(state[0], state[3]);

            state[2] = _mm256_xor_si256(state[2], temp);

            state[3] = rotl(state[3], 45);

            return result;
        }
#else
        class SIMDState
        {
           public:
            SIMDState() {}
            SIMDState(const SIMDState& state_to_copy, JumpOnCopy jump_dist = JumpOnCopy::None) {}
        };
#endif

        SIMDState simd_state_;

        static uint64_t next_internal(SerialState& state)
        {
            const uint64_t result = state[0] + state[3];

            const uint64_t t = state[1] << 17;

            state[2] ^= state[0];
            state[3] ^= state[1];
            state[1] ^= state[2];
            state[0] ^= state[3];

            state[2] ^= t;

            state[3] = rotl(state[3], 45);

            return result;
        }

        static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
        static inline __m256i rotl(const __m256i x, int k)
        {
            return _mm256_or_si256(_mm256_slli_epi64(x, k), _mm256_srli_epi64(x, 64 - k));
        }
    };
}  // namespace SEFUtility::RNG
