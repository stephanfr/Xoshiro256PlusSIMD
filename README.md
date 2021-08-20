# Xoshiro256PlusSIMD
Serial and SIMD implementation of the Xoshiro256+ random number generator.


This project provides a C++ implementation of Xoshiro256+ that matches the performance of the reference C
implementation of David Blackman and Sebastiano Vigna.  Xoshiro256+ combines high speed, small memory
space requirements for stored state and excellent statistical quality.  For cryptographic use cases or
use cases where absolutely the best statistical quality is required - maybe consider a different RNG like
the Mersenne Twist.  For any just about any conventional simulation or testing use case, Xoshiro256+ should be perfectly fine statistically.

This implementation is a header-only library and provides the following capabilities:

    Single 64 bit unsigned random value
    Single 64 bit unsigned random value reduced to a [lower, upper) range
    Four 64 bit unsigned random values
    Four 64 bit unsigned random values reduced to a [lower, upper) range

    Single double length real random value in a range of (0,1)
    Single double length real random value in a (lower, upper) range
    Four double length real random values in a range of (0,1)
    Four double length real random values in a (lower, upper) range

For platforms supporting the AVX2 instruction set, the RNG can be configured to use AVX2 instructions or not on
an instance by instance basis.  AVX2 instructions are only used for the four-wide operations, there is no advantage
using them for single value generation.

The four-wide operations use a different random seed per value and the the seed for single value generation is
distinct as well.  The same stream of values will be returned by the serial and AVX2 implementations.  It *might*
be faster for the serial implementation to use only a single seed across all the four values - each increasing index
being the next value in a single series, instead of each of the four values having its unique series.  The downside
of that approach is that the serial implementation would return different four wide values than the AVX2 implementation.  
The AVX2 implementation must use distinct seeds for each of the four values.

The random series for each of the four-wide values are separated by 2^192 values - i.e. a Xoshiro256+ 'long jump'
separates the seed for each of the four values.  For clarity, the Xoshiro256+ has a state space of 2^256.

The reduction of the uint64s to an integer range takes uint32 bounds.  This is s significant reduction in the size 
of the random values but permits reduction while avoiding taking a modulus.  If you have a need for random
integer values beyond uint32 sizes, I'd suggest taking the full 64 bit values and applying your own reduction
algorithm.  The modulus approach to reduction is slower than the approach in the code which uses shifts and a multiply.

Finally, the AVX versions are coded explicitly with AVX intrinsics, there is no reliance on the vageries of compiler 
vectorization.  The SIMD version could be written such that gcc *should* unroll loops and vectorize but others have
reported that it is necessary to tweak optimization flags to get the unrolling to work.  For these implementations,
all that is needed is to have the -mavx2 compiler option and the __AVX2_AVAILABLE__ symbol defined. 

# Usage

The class Xoshiro256Plus is a template class and takes an SIMDInstructionSet enumerated value as its only
template parameter.  SIMDInstructionSet may be 'NONE', 'AVX' or 'AVX2'.  The SIMD acceleration requires the AVX2
instruction set and uses 'if contexpr' to control code generation at compile time.  There is also a preprocessor
symbol __AVX2_AVAILABLE__ which must be defined to permit AVX2 instances of the RNG to be created.  It it completely
reasonable to have the AVX2 instruction set available but still use an RNG instance with no SIMD acceleration.

    #define __AVX2_AVAILABLE__

    #include "Xoshiro256Plus.h"

    constexpr size_t NUM_SAMPLES = 1000;
    constexpr uint64_t SEED = 1;

    typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::NONE> Xoshiro256PlusSerial;
    typedef SEFUtility::RNG::Xoshiro256Plus<SIMDInstructionSet::AVX2> Xoshiro256PlusAVX2;

    bool    InsureFourWideRandomStreamsMatch()
    {
        Xoshiro256PlusSerial serial_rng(SEED);
        Xoshiro256PlusAVX2 avx_rng(SEED);

        for (auto i = 0; i < NUM_SAMPLES; i++)
        {
            auto next_four_serial = serial_rng.next4( 200, 300 );
            auto next_four_avx = avx_rng.next4( 200, 300 );

            if(( next_four_serial[0] != next_four_avx[0] ) ||
               ( next_four_serial[1] != next_four_avx[1] ) ||
               ( next_four_serial[2] != next_four_avx[2] ) ||
               ( next_four_serial[3] != next_four_avx[3] ))
            {
                return false;
            }
        }

        return true;
    }

As can be seen in the above example, four wide random values are created by both a fully serial instance
as well as by an AVX2 SIMD instance.

# Benchmarks

The AVX2 flavor of the RNG is clearly faster than the serial version - but only if you need 3 or more random values
at the same time.  For a single stream of random values, the AVX2 flavor of the generator is slower.  A full combination of benchmarks appears below.

The benchmarks generate the same total number of random values.  For example, the serial benchmark
creates 1,000,000 values and the four wide calls create 250,000 four wide results for the same total number of values.  The first benchmark is a 'reference' which uses the C implementation by Blackman and Vigna.  As can be seen, there is no penalty for using the C++ class - the timing compared to the reference is virtually identical.

Generating four wide results using the serial generator is not 4x the time of generating serial values, likely due to the CPU
being able to keep the RNG state in high speed cache or perhaps a register the whole time.  The AVX accelerated four wide
performance is roughly 2x that of the four wide serial implementation.  This matches fairly well with results from others.

The benchmarks include timing for unbounded uint64s, bounded uint64s, unbounded doubles and bounded doubles.  In general,
the unbounded uint64 performance is best and the other permutations share similar performance.

There also exist benchmarks where the values are generated and discarded, the values are summed using a single
variable or values are summed into __m256i packed integer or __m256d packed double variables using AVX SIMD instructions.  In general, the sums using the AVX SIMD instructions are faster than the same using a single variable (which ought to be the case).

A final note on performance - do not expect big performance boosts **unless** your code uses the AVX2 implementation and then
continues to work with the returned values using additional AVX instructions.  Any performance gain accrued by using the
AVX2 SIMD accelerated RNG is lost if the four wide values are then extracted individually and used one by one.

The following benchmarks were generated on 2 core Ubunutu 20.04 VM running on a Ryzen 7 1700 HW platform.  I expect the results
will generally be the same on other AMD Zen platforms and I expect the relative performance of the different permutations to
be generally the same on recent Intel CPUs but..... YMMV.  The only way to know for sure if your code will benefit from the SIMD
implementation is to benchmark it on your target platform(s).

Times are for generating 1,000,000 values.  For the reference benchmark and the first serial unbounded uint64 benchmark,
a little more than 1ms was required to generate the 1M values, therefore the RNG is able to generarte a new value in about 1ns
on the target system.  By comparison, the AVX2 implementation was able to generate 250k four wide values in 526 microseconds, which is about half a nanosecond per value.

    -------------------------------------------------------------------------------
    Benchmarks
    -------------------------------------------------------------------------------
    /home/steve/dev/xoroshiro/Xoshiro256PlusSIMD/UnitTest/Benchmark.cpp:15
    ...............................................................................

    benchmark name                       samples       iterations    estimated
                                        mean          low mean      high mean
                                        std dev       low std dev   high std dev
    -------------------------------------------------------------------------------
    Reference                                      100             1    108.357 ms 
                                            1.09005 ms     1.0857 ms     1.1014 ms 
                                            32.5232 us    7.15382 us    63.1828 us 
                                                                                
    Serial next()                                  100             1    108.376 ms 
                                            1.08375 ms    1.08288 ms    1.08564 ms 
                                            6.23032 us    3.62088 us    12.3273 us 
                                                                                
    Serial next() Bounded                          100             1    135.419 ms 
                                            1.37487 ms    1.37148 ms    1.37826 ms 
                                            17.2638 us    16.1752 us    18.4977 us 
                                                                                
    Serial dnext()                                 100             1    97.6651 ms 
                                            969.915 us    968.114 us    972.931 us 
                                            11.6483 us    7.06496 us    18.5479 us 
                                                                                
    Serial dnext() bounded                         100             1    97.7745 ms 
                                            972.049 us    969.954 us    975.171 us 
                                            12.8645 us    9.52305 us    18.6562 us 
                                                                                
    Serial next4() no sums                         100             1    192.779 ms 
                                            1.9285 ms    1.92715 ms    1.93025 ms 
                                            7.83191 us    6.36283 us    10.6269 us 
                                                                                
    Serial next4() sum in __m256i                  100             1    97.8536 ms 
                                            973.69 us    971.788 us    977.198 us 
                                            12.7489 us     8.1987 us    24.1979 us 
                                                                                
    Serial next4() sum in uint64_t                 100             1    276.699 ms 
                                            2.76651 ms     2.7649 ms    2.76976 ms 
                                            11.2316 us    6.77214 us    21.0194 us 
                                                                                
    Serial next4() bounded sum in                                                  
    __m256i                                        100             1    144.767 ms 
                                            1.4479 ms    1.44679 ms    1.44927 ms 
                                            6.27935 us    5.21632 us    7.65162 us 
                                                                                
    Serial next4() bounded sum in                                                  
    uint64_t                                       100             1     235.09 ms 
                                            2.35054 ms    2.34782 ms    2.35502 ms 
                                            17.485 us      12.03 us    26.1283 us 
                                                                                
    Serial dnext4() sum in __m256d                 100             1    235.221 ms 
                                            2.36124 ms    2.34903 ms    2.41312 ms 
                                            109.93 us    18.6142 us    257.712 us 
                                                                                
    Serial dnext4() sum in double                  100             1    226.814 ms 
                                            2.26487 ms    2.26306 ms    2.26791 ms 
                                            11.7398 us    8.13546 us    19.1333 us 
                                                                                
    Serial dnext4() bounded sum in                                                 
    __m256d                                        100             1     250.89 ms 
                                            2.51657 ms    2.50865 ms    2.52466 ms 
                                            40.906 us    37.7936 us     45.844 us 
                                                                                
    Serial dnext4() bounded sum in                                                 
    double                                         100             1    248.293 ms 
                                            2.47681 ms    2.47396 ms    2.48162 ms 
                                            18.447 us    12.3999 us    27.4373 us 
                                                                                
    AVX next4() no sum                             100             1    53.9242 ms 
                                            526.423 us    525.076 us    528.111 us 
                                            7.6698 us    6.46322 us    9.03552 us 
                                                                                
    AVX next4() sum in _m256i                      100             1    60.9563 ms 
                                            593.59 us    591.108 us    596.694 us 
                                            14.0881 us     11.872 us    17.1439 us 
                                                                                
    AVX next4() sum in uint64_t                    100             1    86.0758 ms 
                                            859.873 us    858.773 us    862.098 us 
                                            7.66943 us    4.52627 us    14.6299 us 
                                                                                
    AVX next4() bounded sum in _m256i              100             1    84.0304 ms 
                                            835.803 us    834.757 us    837.257 us 
                                            6.26968 us    4.76359 us    8.78927 us 
                                                                                
    AVX next4() bounded sum in uint64_t            100             1    102.108 ms 
                                            1.0234 ms    1.02124 ms    1.02678 ms 
                                            13.5079 us    9.35676 us     18.207 us 
                                                                                
    AVX dnext4() sum in __m256d                    100             1    96.4095 ms 
                                            955.437 us    950.583 us    960.529 us 
                                            25.3652 us    22.9326 us    29.2549 us 
                                                                                
    AVX dnext4() sum in double                     100             1    109.479 ms 
                                            1.12603 ms    1.09848 ms    1.25363 ms 
                                            257.263 us     23.676 us    610.122 us 
                                                                                
    AVX dnext4() bounded sum in __m256d            100             1    104.927 ms 
                                            1.05451 ms    1.05284 ms    1.05718 ms 
                                            10.6023 us    6.97456 us    16.3411 us 
                                                                                
    AVX dnext4() bounded sum in double             100             1    125.021 ms 
                                            1.24846 ms    1.24757 ms    1.25021 ms 
                                            6.16257 us    3.78216 us    11.4779 us 
                                                                               

# Perfomance without AVX2 at all

A final set of benchmarks below were generated for a subset of tests with the -mavx2 compiler flag not supplied.

This models a platform without the AVX2 instruction set at all and eliminates a variety of optimizations
the compiler *can* apply to the serial RNG code - even without a fully SIMD RNG implementation.  Essentially,
even on the same platform, the compiler can accelerate the serial code using AVX/AVX2 instructions even if
the RNG instance is not using the AVX2 intrinsics explicitly.

As can be seen, the reference and serial timing is the same - it has no dependency on the availability of AVX2 
instructions.   The biggest difference is seen with the double benchmark - it is considerably slower than the serial
implementation.  It is possible that could be addressed with a bit more careful coding but if you are planning on doing
anything that requires high performance numerical processing and you need a high-speed stream of random values - 
you ought to be running on a processer with AVX2, pretty much any decent processor made in the last 5 years supports it.

    -------------------------------------------------------------------------------
    Benchmarks No AVX
    -------------------------------------------------------------------------------
    /home/steve/dev/xoroshiro/Xoshiro256PlusSIMD/UnitTestNoAVX/BenchmarkNoAVX.cpp:14
    ...............................................................................

    benchmark name                       samples       iterations    estimated
                                        mean          low mean      high mean
                                        std dev       low std dev   high std dev
    -------------------------------------------------------------------------------
    Reference                                      100             1    108.411 ms 
                                            1.08495 ms    1.08335 ms    1.08858 ms 
                                            11.5081 us    6.45596 us    23.0117 us 
                                                                                
    Serial                                         100             1    108.402 ms 
                                            1.08445 ms    1.08307 ms    1.08644 ms 
                                            8.36526 us    6.29509 us    11.4643 us 
                                                                                
    Serial Bounded                                 100             1     136.32 ms 
                                            1.3555 ms    1.35189 ms    1.35878 ms 
                                            17.5785 us    14.9616 us    20.4743 us 
                                                                                
    Serial dnext()                                 100             1    135.424 ms 
                                            1.35392 ms    1.35193 ms    1.35656 ms 
                                            11.6229 us    9.34572 us    16.6434 us 
                                                                                
    Serial next4() no sums                         100             1    193.046 ms 
                                            1.92718 ms    1.92575 ms    1.92893 ms 
                                            8.08577 us    6.82338 us    10.5066 us 
                                                                                
    Serial next4() sum in uint64_t                 100             1    276.782 ms 
                                            2.76459 ms    2.76303 ms    2.76658 ms 
                                            8.98594 us    7.18895 us    11.8457 us 
                                                                                
    Serial next4() bounded sum in                                                  
    uint64_t                                       100             1    234.484 ms 
                                            2.34598 ms     2.3443 ms    2.34894 ms 
                                            11.0648 us    6.84212 us    18.8356 us 
                                                                                
    Serial dnext4() sum in double                  100             1    636.916 ms 
                                            6.38745 ms    6.38005 ms    6.39681 ms 
                                            42.2024 us    31.7888 us     68.587 us 
                                                                                
# Conclusion

All of the source code, unit tests and benchmarks are available in the repository.  Including the SIMD RNG into 
your project does not guarantee a dramatic performance improvement or even a measurable performance improvement.  
The Xoshiro256+ RNG is so fast that if you are using it already, the rest of your code is likely going 
to be consuming the overwhelming bulk of the CPU cycles.  Moving to the SIMD version will not net you much if the 
rest of your code drawing the random values is not AVX2 friendly already.  If however, you are using a slow RNG 
then using this code and shifting to the Xoshiro256+ RNG may be noticeable and it is possible that the compiler
may be able to better vectorize your code and take advantage of the SIMD RNG for further performance boosts.
