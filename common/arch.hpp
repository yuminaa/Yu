#pragma once

#ifndef YUMINA_ARCH_HPP
#define YUMINA_ARCH_HPP

/**
 * @file arch.hpp
 * @brief Architecture and platform detection macros for the Yumina Engine.
 *
 * This file contains macros for identifying the operating system, architecture,
 * compiler, and various optimizations available for performance tuning.
 */

// Platform Detection
/**
 * @def YUMINA_OS_WINDOWS
 * Defined if the target platform is Windows.
 */
#if defined(_WIN32) || defined(_WIN64)
    #define YUMINA_OS_WINDOWS
#endif

/**
 * @def YUMINA_OS_MACOS
 * Defined if the target platform is macOS.
 */
#if defined(__APPLE__) && defined(__MACH__)
    #define YUMINA_OS_MACOS
#endif

/**
 * @def YUMINA_OS_LINUX
 * Defined if the target platform is Linux.
 */
#if defined(__linux__)
    #define YUMINA_OS_LINUX
#endif

// Architecture Detection
/**
 * @def YUMINA_ARCH_X64
 * Defined if the target architecture is x86_64.
 */
#if defined(__x86_64__) || defined(_M_X64)
    #define YUMINA_ARCH_X64
#endif

/**
 * @def YUMINA_ARCH_ARM64
 * Defined if the target architecture is ARM64.
 */
#if defined(__aarch64__) || defined(_M_ARM64)
    #ifdef YUMINA_ARCH_ARM64
        #undef YUMINA_ARCH_ARM64
    #endif
    #define YUMINA_ARCH_ARM64
#endif

// Compiler Detection
/**
 * @def YUMINA_COMPILER_CLANG
 * Defined if the compiler is Clang.
 */
#if defined(__clang__)
    #define YUMINA_COMPILER_CLANG
#endif

/**
 * @def YUMINA_COMPILER_MSVC
 * Defined if the compiler is Microsoft Visual C++.
 */
#if defined(_MSC_VER)
    #define YUMINA_COMPILER_MSVC
#endif

/**
 * @def YUMINA_COMPILER_GCC
 * Defined if the compiler is GCC.
 */
#if defined(__GNUC__) && !defined(__clang__)
    #define YUMINA_COMPILER_GCC
#endif

// SIMD support for x64 architecture
#if defined(YUMINA_ARCH_X64)
    #include <immintrin.h>
    #include <emmintrin.h>
    #include <xmmintrin.h>
    #include <smmintrin.h>
    #include <tmmintrin.h>
    #ifdef __AVX2__
        #include <avx2intrin.h>
    #endif
    #ifdef __AVX512F__
        #include <avx512fintrin.h>
    #endif

    #ifdef _MSC_VER
        #include <intrin.h>
    #endif

    /**
     * @brief Checks if AVX2 instructions are supported on the current CPU.
     *
     * @return True if AVX2 is supported, false otherwise.
     */
    #ifdef __GNUC__
        #include <cpuid.h>
        [[gnu::always_inline]] inline static bool cpu_has_avx2()
        {
            unsigned int eax, ebx, ecx, edx;
            __get_cpuid(7, &eax, &ebx, &ecx, &edx);
            return (ebx & bit_AVX2) != 0;
        }

        /**
         * @brief Checks if AVX512F instructions are supported on the current CPU.
         *
         * @return True if AVX512F is supported, false otherwise.
         */
        [[gnu::always_inline]] inline static bool cpu_has_avx512f()
        {
            unsigned int eax, ebx, ecx, edx;
            __get_cpuid(7, &eax, &ebx, &ecx, &edx);
            return (ebx & bit_AVX512F) != 0;
        }
    #elif defined(_MSC_VER)
        /**
         * @brief Checks if AVX2 instructions are supported on the current CPU (MSVC version).
         *
         * @return True if AVX2 is supported, false otherwise.
         */
        [[gnu::always_inline]] inline static bool cpu_has_avx2()
        {
            int cpuInfo[4];
            __cpuid(cpuInfo, 7);
            return (cpuInfo[1] & (1 << 5)) != 0;
        }

        /**
         * @brief Checks if AVX512F instructions are supported on the current CPU (MSVC version).
         *
         * @return True if AVX512F is supported, false otherwise.
         */
        [[gnu::always_inline]] inline static bool cpu_has_avx512f()
        {
            int cpuInfo[4];
            __cpuid(cpuInfo, 7);
            return (cpuInfo[1] & (1 << 16)) != 0;
        }
    #endif

    // SIMD Operation Definitions for x64
    #ifdef __AVX512F__
        #define SIMD_WIDTH 64
        #define STREAM_STORE(addr, val) _mm512_stream_si512((__m512i*)(addr), val)
        #define LOAD_VECTOR(addr) _mm512_loadu_si512((const __m512i*)(addr))
        #define STORE_VECTOR(addr, val) _mm512_storeu_si512((__m512i*)(addr), val)
        #define SET_ZERO_VECTOR() _mm512_setzero_si512()
        #define ADD_PS(a, b) _mm512_add_ps(a, b)
        #define MUL_PS(a, b) _mm512_mul_ps(a, b)
        #define FMA_PS(a, b, c) _mm512_fmadd_ps(a, b, c)
    #elif defined(__AVX2__)
        #define SIMD_WIDTH 32
        #define STREAM_STORE(addr, val) _mm256_stream_si256((__m256i*)(addr), val)
        #define LOAD_VECTOR(addr) _mm256_loadu_si256((const __m256i*)(addr))
        #define STORE_VECTOR(addr, val) _mm256_storeu_si256((__m256i*)(addr), val)
        #define SET_ZERO_VECTOR() _mm256_setzero_si256()
        #define ADD_PS(a, b) _mm256_add_ps(a, b)
        #define MUL_PS(a, b) _mm256_mul_ps(a, b)
        #define FMA_PS(a, b, c) _mm256_fmadd_ps(a, b, c)
    #else
        #define SIMD_WIDTH 16
        #define STREAM_STORE(addr, val) _mm_stream_si128((__m128i*)(addr), val)
        #define LOAD_VECTOR(addr) _mm_loadu_si128((const __m128i*)(addr))
        #define STORE_VECTOR(addr, val) _mm_storeu_si128((__m128i*)(addr), val)
        #define SET_ZERO_VECTOR() _mm_setzero_si128()
        #define ADD_PS(a, b) _mm_add_ps(a, b)
        #define MUL_PS(a, b) _mm_mul_ps(a, b)
        #define FMA_PS(a, b, c) _mm_add_ps(_mm_mul_ps(a, b), c)
    #endif

    // Common x86_64 operations
    #define STREAM_STORE_64(addr, val) _mm_stream_si64((__int64*)(addr), val)

// SIMD support for ARM64 architecture
#elif defined(YUMINA_ARCH_ARM64)
    #include <arm_neon.h>
    #ifdef __clang__
        #include <arm_acle.h>
    #endif

    // SVE support
    #ifdef __ARM_FEATURE_SVE
        #include <arm_sve.h>
        #define HAS_SVE
    #endif

    #define SIMD_WIDTH 16

    #if defined(__aarch64__)
        // 64-bit ARM specific optimizations
        #define STREAM_STORE(addr, val) vst1q_u8((uint8_t*)(addr), val)
        #define LOAD_VECTOR(addr) vld1q_u8((const uint8_t*)(addr))
        #define STORE_VECTOR(addr, val) vst1q_u8((uint8_t*)(addr), val)
        #define SET_ZERO_VECTOR() vdupq_n_u8(0)
        #define STREAM_STORE_64(addr, val) vst1_u64((uint64_t*)(addr), vcreate_u64(val))

        #define ADD_PS_F(a, b) vaddq_f32(a, b)
        #define MUL_PS_F(a, b) vmulq_f32(a, b)
        #define FMA_PS_F(a, b, c) vfmaq_f32(c, a, b)

        // Integer operations
        #define ADD_PS_I(a, b) vaddq_s64(a, b)
        #define MUL_PS_I(a, b) vmulq_s32(a, b)

        #define PREFETCH_KEEP(addr) __asm__ volatile("prfm pldl1keep, [%0]" :: "r"(addr))
        #define PREFETCH_STREAM(addr) __asm__ volatile("prfm pldl1strm, [%0]" :: "r"(addr))
        #define PREFETCH_L1(addr) __asm__ volatile("prfm pldl1keep, [%0]" :: "r"(addr))
        #define PREFETCH_L2(addr) __asm__ volatile("prfm pldl2keep, [%0]" :: "r"(addr))
        #define PREFETCH_L3(addr) __asm__ volatile("prfm pldl3keep, [%0]" :: "r"(addr))

    #else
        #define STREAM_STORE(addr, val) vst1q_u8((uint8_t*)(addr), val)
        #define LOAD_VECTOR(addr) vld1q_u8((const uint8_t*)(addr))
        #define STORE_VECTOR(addr, val) vst1q_u8((uint8_t*)(addr), val)
        #define SET_ZERO_VECTOR() vdupq_n_u8(0)
        #define STREAM_STORE_64(addr, val) *((int64_t*)(addr)) = val

        #define ADD_PS(a, b) vaddq_f32(a, b)
        #define MUL_PS(a, b) vmulq_f32(a, b)
        #define FMA_PS(a, b, c) vmlaq_f32(c, a, b)

        #define PREFETCH_KEEP(addr) __pld(reinterpret_cast<const char*>(addr))
        #define PREFETCH_STREAM(addr) __pld(reinterpret_cast<const char*>(addr))
    #endif

#else
    // Generic fallback for unsupported architectures
    #define SIMD_WIDTH 8
    #define STREAM_STORE(addr, val) *((int64_t*)(addr)) = val
    #define LOAD_VECTOR(addr) *((const int64_t*)(addr))
    #define STORE_VECTOR(addr, val) *((int64_t*)(addr)) = val
    #define SET_ZERO_VECTOR() 0
    #define STREAM_STORE_64(addr, val) *((int64_t*)(addr)) = val
    #define ADD_PS(a, b) ((a) + (b))
    #define MUL_PS(a, b) ((a) * (b))
    #define FMA_PS(a, b, c) ((a) * (b) + (c))
#endif

/**
 * @brief Branch prediction optimization macros
 */
#if defined(__clang__)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define ALWAYS_INLINE [[gnu::always_inline]] inline
    #define NEVER_INLINE __attribute__((noinline))
    #define RESTRICT __restrict__
    #define ALIGN_TO(x) __attribute__((aligned(x)))
    #define PACKED __attribute__((packed))
    #define ASSUME(x) __builtin_assume(x)
    #define ASSUME_ALIGNED(x, a) __builtin_assume_aligned(x, a)
    #define NO_SANITIZE __attribute__((no_sanitize("address")))
    #define VECTORIZE_LOOP _Pragma("clang loop vectorize(enable) interleave(enable)")
    #define UNROLL_LOOP _Pragma("clang loop unroll(full)")
    #define NO_VECTORIZE _Pragma("clang loop vectorize(disable)")
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
    #define ALWAYS_INLINE __forceinline
    #define NEVER_INLINE __declspec(noinline)
    #define RESTRICT __restrict
    #define ALIGN_TO(x) __declspec(align(x))
    #define PACKED
    #define ASSUME(x) __assume(x)
    #define ASSUME_ALIGNED(x, a) (x)
    #define NO_SANITIZE
    #define VECTORIZE_LOOP
    #define UNROLL_LOOP
    #define NO_VECTORIZE
#endif

/**
 * @brief Memory and CPU operations
 */
#if defined(YUMINA_ARCH_X64)
    #define MEMORY_FENCE() _mm_mfence()
    #define STORE_FENCE() _mm_sfence()
    #define LOAD_FENCE() _mm_lfence()
    #define CPU_PAUSE() _mm_pause()
    #define CACHE_LINE_SIZE 64

    #define PREFETCH_RW(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
    #define PREFETCH_RO(addr) _mm_prefetch((const char*)(addr), _MM_HINT_NTA)

#elif defined(YUMINA_ARCH_ARM64)
    #define MEMORY_FENCE() asm volatile("dmb ish" ::: "memory")
    #define STORE_FENCE() asm volatile("dmb ishst" ::: "memory")
    #define LOAD_FENCE() asm volatile("dmb ishld" ::: "memory")
    #define CPU_PAUSE() asm volatile("yield" ::: "memory")
    #define CACHE_LINE_SIZE 64

    // ARM-specific cache operations
    #define CACHE_CLEAN(addr) asm volatile("dc cvac, %0" :: "r"(addr))
    #define CACHE_INVALIDATE(addr) asm volatile("dc ivac, %0" :: "r"(addr))
    #define CACHE_CLEAN_AND_INVALIDATE(addr) asm volatile("dc civac, %0" :: "r"(addr))

    // Data synchronization barrier
    #define DSB() asm volatile("dsb sy" ::: "memory")
    #define DSB_ST() asm volatile("dsb st" ::: "memory")
    #define DSB_LD() asm volatile("dsb ld" ::: "memory")

    // Instruction synchronization barrier
    #define ISB() asm volatile("isb" ::: "memory")
#endif

/**
 * @brief Memory alignment macros
 */
#define CACHE_ALIGNED alignas(CACHE_LINE_SIZE)
#define SIMD_ALIGNED alignas(SIMD_WIDTH)

/**
 * @brief Branch prediction optimization macros
 */
#if defined(__clang__) || defined(__GNUC__)
    #define EXPECT(x, val) __builtin_expect_with_probability((x), (val), 1.0)
    #define UNPREDICTABLE(x) __builtin_expect_with_probability((x), 0, 0.5)
#else
    #define EXPECT(x, val) (x)
    #define UNPREDICTABLE(x) (x)
#endif

/**
 * @brief ARM feature detection macros
 */
#if defined(YUMINA_ARCH_ARM64)
    #if defined(__ARM_FEATURE_CRC32)
        #define HAS_CRC32
    #endif
    #if defined(__ARM_FEATURE_CRYPTO)
        #define HAS_CRYPTO
    #endif
    #if defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
        #define HAS_FP16
    #endif
    #if defined(__ARM_FEATURE_BF16_VECTOR_ARITHMETIC)
        #define HAS_BF16
    #endif
    #if defined(__ARM_FEATURE_DOTPROD)
        #define HAS_DOTPROD
    #endif
    #if defined(__ARM_FEATURE_MATMUL_INT8)
        #define HAS_MATMUL_INT8
    #endif
#endif

/**
 * @brief No optimize away macro
 * Prevents compiler from optimizing away a variable
 */
#if defined(__clang__) || defined(__GNUC__)
    #define NO_OPTIMIZE_AWAY(var) asm volatile("" : "+m"(var) : : "memory")
#else
    #define NO_OPTIMIZE_AWAY(var) ::_ReadWriteBarrier()
#endif

/**
 * @brief Function optimization attributes
 */
#if defined(__GNUC__) || defined(__clang__)
    #define NO_SANITIZE __attribute__((no_sanitize("address")))
    #define HOT_FUNCTION __attribute__((hot))
    #define COLD_FUNCTION __attribute__((cold))
    #define PURE_FUNCTION __attribute__((pure))
    #define CONST_FUNCTION __attribute__((const))
#else
    #define NO_SANITIZE
    #define HOT_FUNCTION
    #define COLD_FUNCTION
    #define PURE_FUNCTION
    #define CONST_FUNCTION
#endif

#endif