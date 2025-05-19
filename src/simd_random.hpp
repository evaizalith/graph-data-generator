#ifndef EVA_SIMD_PHILOX
#define EVA_SIMD_PHILOX

#include <immintrin.h>
#include <cstdint>
#include <array>

/*! The purpose of this implementation is to generate and store a large table of random numbers
 * very quickly and efficiently. To do this, we make use of a vectorized Philox4x32 implementation. 
 * This program uses a huge number of random numbers when generating graphs, so it makes sense to
 * generate them in bulk and then cache them instead of generating them on demand. 
 */

class CachedPhiloxAVX2 {
public:
    CachedPhiloxAVX2(uint64_t seed);
    uint32_t operator()();              //!< Returns a random number from the cache

private:
    alignas(32) std::array<__m256i, 2> keys;
    __m256i counter;

    const int rounds = 10;
    const uint32_t multiplier = 0x9E377B9B9;
    const int cache_size = 4096; 
    std::array<uint32_t, 4096> cache;
    int cursor; 

    __m256i generate();                //!< Generates 1 256-bit random number (8 32-bit numbers). This function is compiled differently depending on whether the CPU is Intel or AMD because it makes use of SIMD instructions, and the exact AVX2 instructions change between those two vendors
    void    generateTable();           //!< Populates random number cache 
};

#endif
