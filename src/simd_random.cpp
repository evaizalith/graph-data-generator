#include "simd_random.hpp"
#include <thread>

CachedPhiloxAVX2::CachedPhiloxAVX2(uint64_t seed) {
    // Split seed into two parts and initialize keys
    uint32_t seed1 = static_cast<uint32_t>(seed);
    uint32_t seed2 = static_cast<uint32_t>(seed >> 32);

    keys[0] = _mm256_set_epi32(seed2, seed1, seed2, seed1, seed2, seed1, seed2, seed1);
    keys[1] = _mm256_set_epi32(seed1, seed2, seed1, seed2, seed1, seed2, seed1, seed2);

    counter = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    cursor = 0;

    std::thread thread(generateTable, std::ref(counter), std::ref(keys[0]), std::ref(keys[1]), std::ref(cache), cache_size);
    thread.join();
}

uint32_t CachedPhiloxAVX2::operator()() {
    if (cursor >= cache_size - 1) { 
        cursor = 0;
        std::thread thread(generateTable, std::ref(counter), std::ref(keys[0]), std::ref(keys[1]), std::ref(cache), cache_size);
        thread.detach();
    }
    return cache[cursor++];
}

uint32_t CachedPhiloxAVX2::operator()(int max) {
    return this->operator()() % max;
}

__m256i CachedPhiloxAVX2::generate(__m256i& counter, __m256i& key_low, __m256i& key_high, int rounds) {
    __m256i state = counter;
    #ifdef Intel
    for (int i = 0; i < rounds; i++) {
        __m256i mul  = _mm256_mullo_epi32(state, _mm256_set1_epi32(multiplier));
        __m256i high = _mm256_mulhi_epu32(state, _mm256_set1_epi32(multiplier)); 
        __m256i prod = _mm256_xor_si256(mul, high);

        __m256i round_key = (i % 2 == 0) ? key_low : key_high;
        prod = _mm256_xor_si256(prod, round_key);

        state = _mm256_shuffle_epi32(prod, _MM_SHUFFLE(2, 3, 0, 1));
    }

    #else // inline assembly because AMD does not support all of the above intrinsics 
    asm volatile (
        "vmovdqa %[state], %%ymm0\n\t"        // Load state
        "vbroadcastss %[mult], %%ymm4\n\t"    // Broadcast multiplier
        "vmovdqa %[key_lo], %%ymm5\n\t"       // Load keys
        "vmovdqa %[key_hi], %%ymm6\n\t"
        "mov $10, %%ecx\n\t"              
        
        "1:\n\t"
        // Multiply low and approximate high
        "vpmulld %%ymm4, %%ymm0, %%ymm1\n\t"  // Multiply low
        "vpsrld $16, %%ymm0, %%ymm2\n\t"      // Approximate high
        "vpmulld %%ymm4, %%ymm2, %%ymm2\n\t"
        "vpslld $16, %%ymm2, %%ymm2\n\t"
        "vpxor %%ymm1, %%ymm2, %%ymm0\n\t"    // Combine
        
        // Apply round key (alternating keys)
        "test $1, %%ecx\n\t"
        "jz 2f\n\t"
        "vpxor %%ymm6, %%ymm0, %%ymm0\n\t"    // XOR with key_high
        "jmp 3f\n\t"
        "2:\n\t"
        "vpxor %%ymm5, %%ymm0, %%ymm0\n\t"    // XOR with key_low
        "3:\n\t"
        
        // Permute lanes
        "vpshufd $0xB1, %%ymm0, %%ymm0\n\t"   // Shuffle (0xB1 = 2,3,0,1)
        
        "sub $1, %%ecx\n\t"
        "jnz 1b\n\t"
        
        "vmovdqa %%ymm0, %[state]\n\t"        // Store result
        : [state] "+x" (state)
        : [mult] "m" (multiplier),
          [key_lo] "x" (key_low),
          [key_hi] "x" (key_high)
        : "ymm0", "ymm1", "ymm2", "ymm4", "ymm5", "ymm6", "ecx", "cc"
    );
    #endif 

    counter = _mm256_add_epi32(counter, _mm256_set1_epi32(8));
    return state;
}

void CachedPhiloxAVX2::generateTable(__m256i& counter, __m256i& key_low, __m256i& key_high, std::array<std::atomic<uint32_t>, 8192>& cache, const int cache_size) {
    const int NUMBERS_PER_ITERATION = 8;
    const int ITERATIONS = cache_size / NUMBERS_PER_ITERATION;
    //static_assert(cache_size % NUMBERS_PER_ITERATION == 0, "CachedPhiloxAVX2::generateTable(): cache size must be neatly divisible by the number of items per vector"); 

    for (int i = 0; i < ITERATIONS; ++i) {
        __m256i rand_values = generate(std::ref(counter), std::ref(key_low), std::ref(key_high), 10);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(cache.data() + i * 8), rand_values);
    }
}
