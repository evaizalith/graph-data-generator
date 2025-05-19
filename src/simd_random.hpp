#ifndef EVA_SIMD_PHILOX
#define EVA_SIMD_PHILOX

#include <immintrin.h>
#include <cstdint>
#include <array>
#include <atomic>

const uint32_t multiplier = 0x9E377B9B9;

/*! The purpose of this implementation is to generate and store a large table of random numbers
 * very quickly and efficiently. To do this, we make use of a vectorized Philox4x32 implementation. 
 * This program uses a huge number of random numbers when generating graphs, so it makes sense to
 * generate them in bulk and then cache them instead of generating them on demand. 
 */

class CachedPhiloxAVX2 {
public:
    CachedPhiloxAVX2(uint64_t seed);
    uint32_t operator()();              //!< Returns a random number from the cache
    uint32_t operator()(int max);       //!< Returns random number with certain max value

private:
    alignas(32) std::array<__m256i, 2> keys;
    __m256i counter;
    const int rounds = 10;
    const int cache_size = 8192; 
    std::array<std::atomic<uint32_t>, 8192> cache;
    int cursor; 

    __m256i static generate(__m256i& counter, __m256i& key_low, __m256i& key_high, int rounds);                //!< Generates 1 256-bit random number (8 32-bit numbers). This function is compiled differently depending on whether the CPU is Intel or AMD because it makes use of SIMD instructions, and the exact AVX2 instructions change between those two vendors
    void    static generateTable(__m256i& counter, __m256i& key_low, __m256i& key_high, std::array<std::atomic<uint32_t>, 8192>&, const int cache_size);           //!< Populates random number cache 
};

#endif
