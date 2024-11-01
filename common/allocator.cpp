#ifdef _WIN32
    #include <malloc.h>
    #include <windows.h>
    // Windows virtual memory management
    #define MAP_MEMORY(size) VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    #define UNMAP_MEMORY(ptr, size) VirtualFree(ptr, 0, MEM_RELEASE)
    #ifndef MAP_FAILED
        #define MAP_FAILED nullptr
    #endif
    #define ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
    #define ALIGNED_FREE(ptr) _aligned_free(ptr)
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <sys/mman.h>

    #define MAP_MEMORY(size) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define UNMAP_MEMORY(ptr, size) munmap(ptr, size)
    #define ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, size)
    #define ALIGNED_FREE(ptr) free(ptr)

#else
    // POSIX-compliant systems (Linux, BSD, etc.)
    #include <sched.h>
    #include <unistd.h>
    #include <sys/mman.h>
    #include <thread>

    #define MAP_MEMORY(size) \
    mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define UNMAP_MEMORY(ptr, size) munmap(ptr, size)
    #define ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, size)
    #define ALIGNED_FREE(ptr) free(ptr)
#endif

#include "allocator.h"
#include "arch.hpp"

namespace yumina::detail
{
    thread_local thread_cache_t thread_cache_{};
    thread_local auto pool_manager_ = new pool_manager();
    thread_local auto large_block_cache_ = new large_block_cache_t();
    thread_local std::array<tiny_block_manager*, TINY_CLASSES> tiny_pools_{};

    static constexpr size_t get_alignment_for_size(const size_t size) noexcept
    {
        return size <= CACHE_LINE_SIZE
                   ? CACHE_LINE_SIZE
                   : size >= PG_SIZE
                         ? PG_SIZE
                         : 1ULL << (64 - __builtin_clzll(size - 1));
    }

    constexpr std::array<size_class, 32> size_classes = []
    {
        std::array<size_class, 32> classes{};
        for (size_t i = 0; i < 32; ++i)
        {
            const size_t size = 1ULL << (i + 3);
            const size_t alignment = get_alignment_for_size(size);
            const size_t slot = size + alignment - 1 & ~(alignment - 1);
            classes[i] = {
                static_cast<uint16_t>(size),
                static_cast<uint16_t>(slot),
                static_cast<uint16_t>(PG_SIZE / slot),
                static_cast<uint16_t>(slot - size)
            };
        }
        return classes;
    }();

    ALWAYS_INLINE
    void *thread_cache_t::get(const uint8_t size_class) noexcept
    {
        auto &[blocks, count] = caches[size_class];
        if (LIKELY(count > 0))
        {
            PREFETCH_L1(&blocks[count - 2]);
            return blocks[--count].ptr;
        }
        return nullptr;
    }

    bool thread_cache_t::put(void *ptr, const uint8_t size_class) noexcept
    {
        if (auto &[blocks, count] = caches[size_class]; LIKELY(count < CACHE_SIZE))
        {
            blocks[count].ptr = ptr;
            blocks[count].size_class = size_class;
            ++count;
            return true;
        }
        return false;
    }

    void thread_cache_t::clear() noexcept
    {
        for (auto &[blocks, count] : caches)
            count = 0;
    }

    ALWAYS_INLINE static size_t count_trailing_zeros(uint64_t x)
    {
        #if defined(YUMINA_ARCH_X64)
        #ifdef YUMINA_COMPILER_MSVC
            uint64_t index;
            _BitScanForward64(&index, x);
            return static_cast<size_t>(index);
        #else
            return __builtin_ctzll(x);
        #endif
        #elif defined(YUMINA_ARCH_ARM64)
            return __builtin_ctzll(x);
        #else
            if (x == 0)
                return 64;
            size_t n = 0;
            if ((x & 0x00000000FFFFFFFF) == 0)
            {
                n += 32;
                x >>= 32;
            }
            if ((x & 0x000000000000FFFF) == 0)
            {
                n += 16;
                x >>= 16;
            }
            if ((x & 0x00000000000000FF) == 0)
            {
                n += 8;
                x >>= 8;
            }
            if ((x & 0x000000000000000F) == 0)
            {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x0000000000000003) == 0)
            {
                n += 2;
                x >>= 2;
            }
            if ((x & 0x0000000000000001) == 0)
                n += 1;
            return n;
        #endif
    }

    bitmap::bitmap() noexcept
    {
        for (auto& word : words)
            word.store(~0ULL, std::memory_order_relaxed);
    }

    size_t bitmap::find_free_block(size_t size) noexcept
    {
        const size_t alignment = get_alignment_for_size(size);
        const size_t align_mask = alignment / BITS_PER_WORD - 1;

        PREFETCH_L1(&words[0]);

        #if defined(YUMINA_ARCH_X64)
            #ifdef __AVX512F__
                // AVX-512 optimized scan
                for (size_t i = 0; i < WORDS_PER_BITMAP; i += 8)
                {
                    if ((i & align_mask) != 0) continue;
                    __m512i v = _mm512_loadu_si512((__m512i*)(words + i));
                    __mmask8 mask = _mm512_movepi64_mask(v);
                    if (mask)
                    {
                        const int lane = __builtin_ctz(mask);
                        const uint64_t word = words[i + lane].load(std::memory_order_relaxed);
                        const size_t bit = count_trailing_zeros(word);
                        const size_t block_offset = (i + lane) * 64 + bit;
                        if (words[i + lane].compare_exchange_weak(
                            const_cast<uint64_t&>(word),
                            word & ~(1ULL << bit),
                            std::memory_order_acquire,
                            std::memory_order_relaxed))
                        {
                            MEMORY_FENCE();
                            return block_offset;
                        }
                    }
                }
            #elif defined(__AVX2__)
                // AVX2 optimized scan
                for (size_t i = 0; i < WORDS_PER_BITMAP; i += 4)
                {
                    if ((i & align_mask) != 0) continue;
                    __m256i v = _mm256_loadu_si256((__m256i*)(words + i));
                    __m256i zero = _mm256_setzero_si256();
                    __m256i cmp = _mm256_cmpeq_epi64(v, zero);
                    uint32_t mask = ~_mm256_movemask_epi8(cmp);
                    if (mask)
                    {
                        const size_t lane = __builtin_ctz(mask) >> 3;
                        const uint64_t word = words[i + lane].load(std::memory_order_relaxed);
                        const size_t bit = count_trailing_zeros(word);
                        const size_t block_offset = (i + lane) * 64 + bit;
                        if (words[i + lane].compare_exchange_weak(
                            const_cast<uint64_t&>(word),
                            word & ~(1ULL << bit),
                            std::memory_order_acquire,
                            std::memory_order_relaxed))
                        {
                            MEMORY_FENCE();
                            return block_offset;
                        }
                    }
                }
            #endif
        #elif defined(YUMINA_ARCH_ARM64)
            // NEON optimized scan
            for (size_t i = 0; i < WORDS_PER_BITMAP; i += 2)
            {
                if ((i & align_mask) != 0) continue;
                uint64x2_t v = vld1q_u64(reinterpret_cast<const uint64_t*>(words + i));
                uint64x2_t zero = vdupq_n_u64(0);
                if (uint64x2_t mask = vceqq_u64(v, zero); vgetq_lane_u64(mask, 0) != -1ULL)
                {
                    const uint64_t word = words[i].load(std::memory_order_relaxed);
                    const size_t bit = count_trailing_zeros(word);
                    const size_t block_offset = i * 64 + bit;
                    if (words[i].compare_exchange_weak(
                        const_cast<uint64_t&>(word),
                        word & ~(1ULL << bit),
                        std::memory_order_acquire,
                        std::memory_order_relaxed))
                    {
                        MEMORY_FENCE();
                        return block_offset;
                    }
                }
            }
        #endif

        VECTORIZE_LOOP
        for (size_t i = 0; i < WORDS_PER_BITMAP; ++i)
        {
            if ((i & align_mask) != 0)
                continue;
            uint64_t expected = words[i].load(std::memory_order_relaxed);
            while (expected != 0)
            {
                const size_t bit = count_trailing_zeros(expected);
                const size_t block_offset = i * BITS_PER_WORD + bit;
                if (const uint64_t desired = expected & ~(1ULL << bit);
                    words[i].compare_exchange_weak(
                        expected,
                        desired,
                        std::memory_order_acquire,
                        std::memory_order_relaxed))
                {
                    MEMORY_FENCE();
                    return block_offset;
                }
            }
        }
        return ~static_cast<size_t>(0);
    }

    void bitmap::mark_free(size_t index) noexcept
    {
        const size_t word_idx = index / BITS_PER_WORD;
        const size_t bit_idx = index % BITS_PER_WORD;
        PREFETCH_L1(&words[word_idx]);
        words[word_idx].fetch_or(1ULL << bit_idx, std::memory_order_release);
    }

    bool bitmap::is_completely_free() const noexcept
    {
        VECTORIZE_LOOP
        for (auto& word : words)
        {
            if (word.load(std::memory_order_relaxed) != ~0ULL)
                return false;
        }
        return true;
    }

    void block_header::init(const size_t sz, const uint8_t size_class, const bool is_free) noexcept
    {
        if (UNLIKELY(sz > 1ULL << 47))
            return;
        data = (sz & SIZE_MASK) |
               static_cast<uint64_t>(size_class) << 48 |
               static_cast<uint64_t>(is_free) << 63;
        magic = HEADER_MAGIC;
    }

    void block_header::encode(const size_t size, const uint8_t size_class) noexcept
    {
        data = (size & SIZE_MASK) |
               static_cast<uint64_t>(size_class) << 48;
    }

    bool block_header::set_free(const bool is_free) noexcept
    {
        data = (data & ~(1ULL << 63)) | static_cast<uint64_t>(is_free) << 63;
        return true;
    }

    bool block_header::is_valid() const noexcept
    {
        return magic == HEADER_MAGIC && (data & MAGIC_MASK) == MAGIC_VALUE;
    }

    void* pool::alloc(const size_class& sc) noexcept
    {
        const size_t size = sc.size;

        if (const size_t index = bitmap.find_free_block(size);
            index != ~static_cast<size_t>(0))
        {
            void* block = memory + index * sc.slot_size;
            auto* header = new(block) block_header();
            header->init(size, SIZE_CLASSES - 1, false);
            return static_cast<char*>(block) + sizeof(block_header);
        }
        return nullptr;
    }

    void pool::free(const void* ptr, const size_class& sc) noexcept
    {
        const size_t offset = static_cast<const char*>(ptr) - reinterpret_cast<const char*>(memory);
        if (const size_t index = offset / sc.slot_size; index < (PG_SIZE - sizeof(bitmap)) / sc.slot_size)
            bitmap.mark_free(index);
    }

    bool pool::is_completely_free() const noexcept
    {
        return bitmap.is_completely_free();
    }

    void pool::return_mem() noexcept
    {
        #ifdef YUMINA_OS_LINUX
            if (auto* current = reinterpret_cast<block_header*>(memory))
            {
                PREFETCH_L1(current);
                size_t free_space = 0;
            #if defined(YUMINA_ARCH_X64) && defined(__AVX512F__)
                __m512i sum = _mm512_setzero_si512();
                while (current)
                {
                    if (current->is_free())
                        sum = _mm512_add_epi64(sum, _mm512_set1_epi64(current->size()));
                    current = current->next_physical;
                }
                free_space = _mm512_reduce_add_epi64(sum);
            #elif defined(YUMINA_ARCH_X64) && defined(__AVX2__)
                __m256i sum1 = _mm256_setzero_si256();
                __m256i sum2 = _mm256_setzero_si256();
                while (current)
                {
                    if (current->is_free())
                    {
                        sum1 = _mm256_add_epi64(sum1, _mm256_set1_epi64x(current->size()));
                        sum2 = _mm256_add_epi64(sum2, _mm256_set1_epi64x(current->size() >> 32));
                    }
                    current = current->next_physical;
                }
                free_space = _mm256_extract_epi64(sum1, 0) + (_mm256_extract_epi64(sum2, 0) << 32);
            #elif defined(YUMINA_ARCH_ARM64)
                uint64x2_t sum = vdupq_n_u64(0);
                while (current)
                {
                    if (current->is_free())
                        sum = vaddq_u64(sum, vdupq_n_u64(current->size()));
                    current = current->next_physical;
                }
                free_space = vgetq_lane_u64(sum, 0) + vgetq_lane_u64(sum, 1);
            #else
                while (current)
                {
                    if (current->is_free())
                        free_space += current->size();
                    current = current->next_physical;
                }
            #endif

                if (free_space >= MIN_RETURN_SIZE &&
                    static_cast<double>(free_space) / PG_SIZE >= (1.0 - MEM_USAGE_THRESHOLD))
                {
                    void* page_start = reinterpret_cast<void*>(
                        (reinterpret_cast<uintptr_t>(memory) + PG_SIZE - 1) & ~(PG_SIZE - 1));
                    void* page_end = memory + (PG_SIZE - sizeof(bitmap));

                    if (page_end > page_start)
                    {
                        madvise(page_start,
                               static_cast<char*>(page_end) - static_cast<char*>(page_start),
                               MADV_DONTNEED);
                    }
                }
            }
            #endif
    }

    pool_manager::pool_manager()
    {
        for (auto& count : pool_count)
            count = 0;
    }

    pool_manager::~pool_manager()
    {
        for (size_t sc = 0; sc < SIZE_CLASSES; ++sc)
        {
            for (size_t i = 0; i < pool_count[sc]; ++i)
                delete pools[sc][i].p;
        }
    }

    pool* pool_manager::alloc_pool(const size_t size_class) noexcept
    {
        if (pool_count[size_class] >= MAX_POOLS)
            return nullptr;

        try
        {
            auto* new_pool = new(std::align_val_t{PG_SIZE}) pool();
            pools[size_class][pool_count[size_class]].p = new_pool;
            pools[size_class][pool_count[size_class]].used_blocks = 0;
            ++pool_count[size_class];
            return new_pool;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void pool_manager::free_pool(const pool* p, const size_t size_class) noexcept
    {
        for (size_t i = 0; i < pool_count[size_class]; ++i)
        {
            if (pools[size_class][i].p == p)
            {
                if (--pools[size_class][i].used_blocks == 0)
                {
                    delete p;
                    --pool_count[size_class];
                    pools[size_class][i] = pools[size_class][pool_count[size_class]];
                }
                break;
            }
        }
    }

    void* tiny_block_manager::tiny_pool::alloc_tiny(const uint8_t size_class) noexcept
    {
        const size_t size = (size_class + 1) << 3;
        const size_t slot_size = size + sizeof(block_header) + ALIGNMENT - 1 & ~(ALIGNMENT - 1);
        const size_t max_blocks = (PG_SIZE - sizeof(bitmap)) / slot_size;

        if (const size_t index = bitmap.find_free_block(size);
            index != ~static_cast<size_t>(0) && index < max_blocks)
        {
            if (uint8_t* block = memory + index * slot_size; block + slot_size <= memory + (PG_SIZE - sizeof(bitmap)))
            {
                auto* header = new(block) block_header();
                header->init(size, size_class, false);
                return block;
            }
        }
        return nullptr;
    }

    void tiny_block_manager::tiny_pool::free_tiny(void *ptr, const uint8_t size_class) noexcept
    {
        const size_t size = (size_class + 1) << 3;
        const size_t slot_size = size + sizeof(block_header) + ALIGNMENT - 1 & ~(ALIGNMENT - 1);
        const size_t offset = static_cast<uint8_t*>(ptr) - memory;

        if (const size_t index = offset / slot_size; index * slot_size < PG_SIZE - sizeof(bitmap))
        {
            if (auto* header = static_cast<block_header*>(ptr); header->is_valid() && header->size_class() == size_class)
            {
                header->set_free(true);
                bitmap.mark_free(index);
            }
        }
    }

    void* tiny_block_manager::alloc_tiny(uint8_t size_class) noexcept
    {
        return pool.alloc_tiny(size_class);
    }

    void tiny_block_manager::free_tiny(void* ptr, uint8_t size_class) noexcept
    {
        pool.free_tiny(ptr, size_class);
    }

    uint64_t large_block_cache_t::get_time() noexcept
    {
        #if defined(__x86_64__)
            return __rdtscp(&aux);
        #elif defined(YUMINA_ARCH_ARM64)
            // Use CNTVCT_EL0 (Virtual Count Register) for ARM64
            uint64_t timestamp;
            asm volatile("mrs %0, cntvct_el0" : "=r"(timestamp));
            return timestamp;
        #else
            // Fallback for other architectures
            return std::chrono::steady_clock::now().time_since_epoch().count();
        #endif
    }

    size_t large_block_cache_t::get_bucket_index(size_t size) noexcept
    {
        #if defined(__x86_64__)
            return (63 - __builtin_clzll(size > MIN_CACHE_BLOCK ? size - 1 : MIN_CACHE_BLOCK)) - 11;
        #elif defined(__aarch64__)
            return (63 - __builtin_clzll(size > MIN_CACHE_BLOCK ? size - 1 : MIN_CACHE_BLOCK)) - 11;
        #else
            size_t index = 0;
            size_t threshold = MIN_CACHE_BLOCK;
            while (threshold < size && index < NUM_BUCKETS - 1)
            {
                threshold <<= 1;
                ++index;
            }
            return index;
        #endif
    }

    void* large_block_cache_t::get_cached_block(size_t size) noexcept
    {
        if (UNLIKELY(size < MIN_CACHE_BLOCK || size > MAX_CACHE_BLOCK))
            return nullptr;

        const size_t bucket_idx = get_bucket_index(size);
        if (UNLIKELY(bucket_idx >= NUM_BUCKETS))
            return nullptr;

        auto& [count2, entries] = buckets[bucket_idx];
        const size_t count = count2.load(std::memory_order_acquire);

        #if defined(YUMINA_ARCH_X64) || defined(YUMINA_ARCH_ARM64)
            PREFETCH_L1(&entries[0]);
            if (count > 1)
                PREFETCH_L1(&entries[1]);
        #endif

        for (size_t i = 0; i < count; ++i)
        {
            auto& entry = entries[i];

            if (void* expected = entry.ptr.load(std::memory_order_relaxed); expected && entry.size >= size && static_cast<double>(entry.size) <= static_cast<double>(size) * MAX_SIZE_RATIO)
            {
                if (entry.ptr.compare_exchange_strong(expected, nullptr,
                    std::memory_order_acquire, std::memory_order_relaxed))
                {
                    count2.fetch_sub(1, std::memory_order_release);
                    total_cached.fetch_sub(entry.size, std::memory_order_relaxed);
                    MEMORY_FENCE();
                    return expected;
                }
            }
        }
        return nullptr;
    }

    bool large_block_cache_t::cache_block(void* ptr, const size_t size) noexcept
    {
        if (UNLIKELY(size < MIN_CACHE_BLOCK || size > MAX_CACHE_BLOCK))
            return false;

        const size_t bucket_idx = get_bucket_index(size);
        if (UNLIKELY(bucket_idx >= NUM_BUCKETS))
            return false;

        if (const size_t current_total = total_cached.load(std::memory_order_relaxed);
            current_total + size > MAX_CACHE_SIZE)
            return false;

        auto&[count1, entries] = buckets[bucket_idx];

        if (const size_t count = count1.load(std::memory_order_acquire); count < size_bucket::BUCKET_SIZE)
        {
            auto&[ptr, size2, last_use] = entries[count];
            if (void* expected = nullptr;
                ptr.compare_exchange_strong(expected, ptr,
                    std::memory_order_release, std::memory_order_relaxed))
            {
                size2 = size;
                last_use = get_time();
                count1.fetch_add(1, std::memory_order_release);
                total_cached.fetch_add(size, std::memory_order_relaxed);
                return true;
            }
        }

        size_t oldest_idx = 0;
        #if defined(YUMINA_ARCH_X64) && defined(__AVX2__)
            __m256i min_time = _mm256_set1_epi64x(UINT64_MAX);
            __m256i indices = _mm256_setr_epi64x(0, 1, 2, 3);

            for (size_t i = 0; i < BUCKET_SIZE; i += 4)
            {
                __m256i times = _mm256_setr_epi64x(
                    bucket.entries[i].last_use,
                    bucket.entries[i+1].last_use,
                    bucket.entries[i+2].last_use,
                    bucket.entries[i+3].last_use
                );
                __m256i mask = _mm256_cmpgt_epi64(min_time, times);
                min_time = _mm256_blendv_epi8(min_time, times, mask);
                indices = _mm256_blendv_epi8(indices,
                         _mm256_setr_epi64x(i, i+1, i+2, i+3), mask);
            }

            alignas(32) uint64_t results[4];
            _mm256_store_si256(reinterpret_cast<__m256i*>(results), min_time);

            for (int i = 0; i < 4; ++i)
            {
                if (results[i] < UINT64_MAX)
                {
                    oldest_idx = i;
                }
            }
        #elif defined(YUMINA_ARCH_ARM64)
            uint64x2_t min_time = vdupq_n_u64(UINT64_MAX);
            uint64x2_t curr_idx = vdupq_n_u64(0);

            for (size_t i = 0; i < size_bucket::BUCKET_SIZE; i += 2)
            {
                uint64x2_t times = vld1q_u64(&entries[i].last_use);
                uint64x2_t mask = vcltq_u64(times, min_time);
                min_time = vbslq_u64(mask, times, min_time);
                curr_idx = vbslq_u64(mask, vdupq_n_u64(i), curr_idx);
            }

            uint64_t times[2], indices[2];
            vst1q_u64(times, min_time);
            vst1q_u64(indices, curr_idx);

            if (times[0] < times[1])
            {
                oldest_idx = indices[0];
            }
            else
            {
                oldest_idx = indices[1];
            }
            #else
            for (size_t i = 0; i < BUCKET_SIZE; ++i)
            {
                if (bucket.entries[i].last_use < UINT64_MAX)
                {
                    oldest_idx = i;
                }
            }
            #endif

        auto& oldest = entries[oldest_idx];

        if (void* expected = oldest.ptr.load(std::memory_order_relaxed); expected && static_cast<double>(size) <= static_cast<double>(oldest.size) * MAX_SIZE_RATIO)
        {
            if (oldest.ptr.compare_exchange_strong(expected, ptr,
                std::memory_order_release, std::memory_order_relaxed))
            {
                total_cached.fetch_sub(oldest.size, std::memory_order_relaxed);
                oldest.size = size;
                oldest.last_use = get_time();
                total_cached.fetch_add(size, std::memory_order_relaxed);
                return true;
            }
        }

        return false;
    }

    void block_header::set_mmapped(const bool is_mmap) noexcept
    {
        data = (data & ~MMAP_FLAG) | static_cast<uint64_t>(is_mmap) << 62;
    }

    size_t block_header::size() const noexcept
    {
        return data & SIZE_MASK;
    }

    uint8_t block_header::size_class() const noexcept
    {
        return (data & CLASS_MASK) >> 48;
    }

    bool block_header::is_free() const noexcept
    {
        return data & 1ULL << 63;
    }

    bool block_header::is_mmapped() const noexcept
    {
        return data & MMAP_FLAG;
    }

    bool block_header::is_aligned() const noexcept
    {
        const size_t sz = size();
        const size_t alignment = sz <= CACHE_LINE_SIZE ?
            CACHE_LINE_SIZE : sz >= PG_SIZE ?
            PG_SIZE : 1ULL << (64 - __builtin_clzll(sz - 1));
        return (reinterpret_cast<uintptr_t>(this) & (alignment - 1)) == 0;
    }

    bool block_header::coalesce() noexcept
    {
        if (is_mmapped() || size_class() < TINY_CLASSES)
            return false;

        auto coalesced = false;
        if (next_physical && next_physical->is_free())
        {
            const size_t combined_size = size() + next_physical->size() + sizeof(block_header);
            next_physical = next_physical->next_physical;
            if (next_physical)
                next_physical->prev_physical = this;
            encode(combined_size, size_class());
            set_coalesced(true);
            coalesced = true;
        }

        if (prev_physical && prev_physical->is_free())
        {
            const size_t combined_size = size() + prev_physical->size() + sizeof(block_header);
            prev_physical->next_physical = next_physical;
            if (next_physical)
                next_physical->prev_physical = prev_physical;
            prev_physical->encode(combined_size, prev_physical->size_class());
            prev_physical->set_coalesced(true);
            coalesced = true;
        }

        return coalesced;
    }

    void block_header::set_coalesced(const bool is_coalesced) noexcept
    {
        data = (data & ~COALESCED_FLAG) | static_cast<uint64_t>(is_coalesced) << 61;
    }

    bool block_header::is_coalesced() const noexcept
    {
        return data & COALESCED_FLAG;
    }

    void large_block_cache_t::clear() noexcept
    {
        for (auto& bucket : buckets)
        {
            const size_t count = bucket.count.load(std::memory_order_acquire);
            for (size_t i = 0; i < count; ++i)
            {
                auto& entry = bucket.entries[i];
                if (void* ptr = entry.ptr.exchange(nullptr, std::memory_order_release))
                {
                    const size_t total_size = entry.size + sizeof(block_header);
                    const size_t alloc_size = total_size + PG_SIZE - 1 & ~(PG_SIZE - 1);
                    UNMAP_MEMORY(static_cast<char*>(ptr) - sizeof(block_header), alloc_size);
                }
            }
            bucket.count.store(0, std::memory_order_release);
        }
        total_cached.store(0, std::memory_order_release);
    }

    ALWAYS_INLINE static void* alloc_tiny(const size_t size) noexcept
    {
        if (UNLIKELY(size == 0))
            return nullptr;

        const uint8_t size_class = (size - 1) >> 3;
        if (UNLIKELY(size_class >= TINY_CLASSES))
            return nullptr;

        auto& tiny_pool = tiny_pools_[size_class];
        if (!tiny_pool)
        {
            try
            {
                tiny_pool = new tiny_block_manager();
            }
            catch (...)
            {
                return nullptr;
            }
        }

        if (void* ptr = tiny_pool->alloc_tiny(size_class))
        {
            auto* header = new(ptr) block_header();
            header->init(size, size_class, false);
            return static_cast<char*>(ptr) + sizeof(block_header);
        }

        return nullptr;
    }

    ALWAYS_INLINE static void* alloc_small(const size_t size) noexcept
    {
        const uint8_t size_class = (size - 1) >> 3;

        if (void* ptr = thread_cache_.get(size_class))
        {
            auto* header = reinterpret_cast<block_header*>(
                static_cast<char*>(ptr) - sizeof(block_header));
            header->set_free(false);
            return ptr;
        }

        if (!pool_manager_)
            pool_manager_ = new pool_manager();

        if (void* ptr = pool_manager_->alloc_pool(size_class))
        {
            auto* header = new (static_cast<char*>(ptr)) block_header();
            header->init(size, size_class, false);
            return static_cast<char*>(ptr) + sizeof(block_header);
        }

        return nullptr;
    }

    ALWAYS_INLINE static void* alloc_medium(const size_t size, const uint8_t size_class) noexcept
    {
        if (void* ptr = thread_cache_.get(size_class))
        {
            auto* header = reinterpret_cast<block_header*>(
                static_cast<char*>(ptr) - sizeof(block_header));
            PREFETCH_L1(header);
            header->set_free(false);
            return ptr;
        }

        if (!pool_manager_)
            pool_manager_ = new pool_manager();

        if (void* ptr = pool_manager_->alloc_pool(size_class))
        {
            auto* header = new (static_cast<char*>(ptr)) block_header();
            header->init(size, size_class, false);
            return static_cast<char*>(ptr) + sizeof(block_header);
        }

        return nullptr;
    }

    ALWAYS_INLINE static void* alloc_large(const size_t size) noexcept
    {
        if (!large_block_cache_)
            large_block_cache_ = new large_block_cache_t();

        if (void* ptr = large_block_cache_->get_cached_block(size))
            return ptr;

        const size_t total_size = size + sizeof(block_header);
        const size_t alloc_size = total_size + PG_SIZE - 1 & ~(PG_SIZE - 1);

        void* ptr = MAP_MEMORY(alloc_size);
        if (UNLIKELY(ptr == MAP_FAILED))
            return nullptr;

        auto* header = new (ptr) block_header();
        header->init(size, 255, false);
        return static_cast<char*>(ptr) + sizeof(block_header);
    }

    [[maybe_unused]]
    ALWAYS_INLINE static void cleanup()
    {
        if (large_block_cache_)
        {
            large_block_cache_->clear();
            delete large_block_cache_;
            large_block_cache_ = nullptr;
        }

        if (pool_manager_)
        {
            delete pool_manager_;
            pool_manager_ = nullptr;
        }

        thread_cache_.clear();
        for (auto& pool : tiny_pools_)
        {
            delete pool;
            pool = nullptr;
        }
    }

    namespace yumina::detail::internal
    {
        void* allocate(const size_t size) noexcept
        {
            if (UNLIKELY(size == 0 || size > 1ULL << 47))
                return nullptr;

            if (LIKELY(size <= TINY_LARGE_THRESHOLD))
                return alloc_tiny(size);

            if (size <= SMALL_LARGE_THRESHOLD)
                return alloc_small(size);

            if (size < LARGE_THRESHOLD)
            {
                const uint8_t size_class = 31 - __builtin_clz(size - 1);
                return alloc_medium(size, size_class);
            }

            return alloc_large(size);
        }

        void deallocate(void* ptr) noexcept
        {
            if (UNLIKELY(!ptr))
                return;

            auto* header = reinterpret_cast<block_header*>(
                static_cast<char*>(ptr) - sizeof(block_header));

            if (UNLIKELY(!header->is_valid()))
                return;

            const auto size_class = header->size_class();

            if (size_class < TINY_CLASSES)
            {
                if (auto* pool = tiny_pools_[size_class])
                    pool->free_tiny(static_cast<char*>(ptr) - sizeof(block_header), size_class);
                return;
            }

            if (size_class == 255)
            {
                if (!large_block_cache_)
                    large_block_cache_ = new large_block_cache_t();

                if (large_block_cache_->cache_block(ptr, header->size()))
                    return;

                const size_t total_size = header->size() + sizeof(block_header);
                const size_t alloc_size = total_size + PG_SIZE - 1 & ~(PG_SIZE - 1);
                UNMAP_MEMORY(static_cast<char*>(ptr) - sizeof(block_header), alloc_size);
                return;
            }

            if (thread_cache_.put(ptr, size_class))
            {
                header->set_free(true);
                return;
            }

            if (pool_manager_)
                pool_manager_->free_pool(
                    reinterpret_cast<pool*>(
                        reinterpret_cast<uintptr_t>(ptr) & ~(PG_SIZE - 1)),
                    size_class);
        }

        void* reallocate(void* ptr, const size_t new_size) noexcept
        {
            if (!ptr)
                return allocate(new_size);

            if (!new_size)
            {
                deallocate(ptr);
                return nullptr;
            }

            const auto* header = reinterpret_cast<block_header*>(
                static_cast<char*>(ptr) - sizeof(block_header));

            if (UNLIKELY(!header->is_valid()))
                return nullptr;

            const size_t old_size = header->size();

            if (const uint8_t old_class = header->size_class(); old_class < TINY_CLASSES)
            {
                if (new_size <= static_cast<size_t>((old_class + 1) << 3))
                    return ptr;
            }
            else if (old_class < SIZE_CLASSES)
            {
                if (new_size <= size_classes[old_class].size)
                    return ptr;
            }

            void* new_ptr = allocate(new_size);
            if (!new_ptr)
                return nullptr;

            const size_t copy_size = std::min(old_size, new_size);
            #if defined(YUMINA_ARCH_X64)
                if (copy_size >= 32)
                {
                    auto src = static_cast<const char*>(ptr);
                    auto dst = static_cast<char*>(new_ptr);

                    #ifdef __AVX512F__
                        // Prefetch for write to improve destination cache line handling
                        PREFETCH_L1(dst);
                        PREFETCH_L1(dst + 64);

                        // Align to 64-byte boundary for optimal AVX-512 performance
                        size_t offset = 0;
                        size_t remaining = copy_size;

                        // Handle unaligned portion
                        size_t dst_align = reinterpret_cast<uintptr_t>(dst) & 63;
                        if (dst_align != 0)
                        {
                            size_t align_fix = 64 - dst_align;
                            std::memcpy(dst, src, align_fix);
                            offset += align_fix;
                            remaining -= align_fix;
                        }

                        for (; remaining >= 64; remaining -= 64, offset += 64)
                        {
                            // Prefetch next iterations
                            PREFETCH_L1(src + offset + 128);
                            PREFETCH_L1(dst + offset + 128);

                            __m512i v = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(src + offset));
                            _mm512_stream_si512(reinterpret_cast<__m512i*>(dst + offset), v);
                        }

                        if (remaining > 0)
                            std::memcpy(dst + offset, src + offset, remaining);

                        MEMORY_FENCE();

                    #elif defined(__AVX2__)
                        PREFETCH_L1(dst);
                        PREFETCH_L1(dst + 32);

                        size_t offset = 0;
                        size_t remaining = copy_size;

                        // Align to 32-byte boundary
                        size_t dst_align = reinterpret_cast<uintptr_t>(dst) & 31;
                        if (dst_align != 0)
                        {
                            size_t align_fix = 32 - dst_align;
                            std::memcpy(dst, src, align_fix);
                            offset += align_fix;
                            remaining -= align_fix;
                        }

                        // Main AVX2 copy loop
                        for (; remaining >= 32; remaining -= 32, offset += 32)
                        {
                            PREFETCH_L1(src + offset + 64);
                            PREFETCH_L1(dst + offset + 64);

                            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + offset));
                            _mm256_stream_si256(reinterpret_cast<__m256i*>(dst + offset), v);
                        }

                        // Handle remaining bytes
                        if (remaining > 0)
                            std::memcpy(dst + offset, src + offset, remaining);

                        MEMORY_FENCE();
                    #endif
                }
                else
                {
                    std::memcpy(new_ptr, ptr, copy_size);
                }
            #elif defined(YUMINA_ARCH_ARM64)
                if (copy_size >= 16)
                {
                    auto src = static_cast<const char*>(ptr);
                    auto dst = static_cast<char*>(new_ptr);

                    // Prefetch destination cache lines
                    PREFETCH_L1(dst);
                    PREFETCH_L1(dst + 64);

                    size_t offset = 0;
                    size_t remaining = copy_size;

                    if (size_t dst_align = reinterpret_cast<uintptr_t>(dst) & 15; dst_align != 0)
                    {
                        size_t align_fix = 16 - dst_align;
                        std::memcpy(dst, src, align_fix);
                        offset += align_fix;
                        remaining -= align_fix;
                    }

                    // Main NEON copy loop
                    for (; remaining >= 32; remaining -= 32, offset += 32)
                    {
                        // Prefetch next chunks
                        PREFETCH_L1(src + offset + 64);
                        PREFETCH_L1(dst + offset + 64);

                        // Load and store using NEON intrinsics
                        uint8x16_t v1 = vld1q_u8(reinterpret_cast<const uint8_t*>(src + offset));
                        uint8x16_t v2 = vld1q_u8(reinterpret_cast<const uint8_t*>(src + offset + 16));
                        vst1q_u8(reinterpret_cast<uint8_t*>(dst + offset), v1);
                        vst1q_u8(reinterpret_cast<uint8_t*>(dst + offset + 16), v2);
                    }

                    // Handle 16-byte chunks
                    for (; remaining >= 16; remaining -= 16, offset += 16)
                    {
                        uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t*>(src + offset));
                        vst1q_u8(reinterpret_cast<uint8_t*>(dst + offset), v);
                    }

                    if (remaining > 0)
                        std::memcpy(dst + offset, src + offset, remaining);
                    MEMORY_FENCE();
                }
                else
                {
                    std::memcpy(new_ptr, ptr, copy_size);
                }
            #else
                std::memcpy(new_ptr, ptr, copy_size);
            #endif

            deallocate(ptr);
            return new_ptr;
        }

        void* callocate(const size_t num, const size_t size) noexcept
        {
            if (UNLIKELY(num == 0 || size == 0))
                return nullptr;

            if (UNLIKELY(num > SIZE_MAX / size))
                return nullptr;

            const size_t total = num * size;
            void* ptr = allocate(total);

            if (LIKELY(ptr))
            {
                #if defined(YUMINA_ARCH_X64)
                    if (total >= 32)
                    {
                        auto dst = static_cast<char*>(ptr);
                        #ifdef __AVX512F__
                            __m512i zero = _mm512_setzero_si512();
                            for (size_t i = 0; i < total - 63; i += 64)
                                _mm512_storeu_si512(reinterpret_cast<__m512i*>(dst + i), zero);
                        #elif defined(__AVX2__)
                            __m256i zero = _mm256_setzero_si256();
                            for (size_t i = 0; i < total - 31; i += 32)
                                _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), zero);
                        #endif
                        std::memset(dst + (total & ~31), 0, total & 31);
                    }
                    else
                    {
                        std::memset(ptr, 0, total);
                    }
                #elif defined(YUMINA_ARCH_ARM64)
                    if (total >= 16)
                    {
                        auto dst = static_cast<char*>(ptr);
                        uint8x16_t zero = vdupq_n_u8(0);
                        for (size_t i = 0; i < total - 15; i += 16)
                            vst1q_u8(reinterpret_cast<uint8_t*>(dst + i), zero);
                        std::memset(dst + (total & ~15), 0, total & 15);
                    }
                    else
                    {
                        std::memset(ptr, 0, total);
                    }
                #else
                    std::memset(ptr, 0, total);
                #endif
            }
            return ptr;
        }

        void cleanup() noexcept
        {
        }

        void thread_cleanup() noexcept
        {
            cleanup();
        }
    }
}

[[nodiscard]]
void* operator new(const size_t __sz) // NOLINT(*-reserved-identifier)
{
    return yumina::detail::yumina::detail::internal::allocate(__sz);
}

[[nodiscard]]
void* operator new[](const size_t __sz) // NOLINT(*-reserved-identifier)
{
    return yumina::detail::yumina::detail::internal::allocate(__sz);
}

void operator delete(void* __p) noexcept // NOLINT(*-reserved-identifier)
{
    yumina::detail::yumina::detail::internal::deallocate(__p);
}

void operator delete[](void* __p) noexcept // NOLINT(*-reserved-identifier)
{
    yumina::detail::yumina::detail::internal::deallocate(__p);
}
