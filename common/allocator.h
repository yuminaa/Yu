#ifndef YUMINA_INTERNAL_ALLOCATOR_H
#define YUMINA_INTERNAL_ALLOCATOR_H

#include <array>
#include <cstdint>
#include "arch.hpp"

namespace yumina::detail
{
    static constexpr size_t PG_SIZE = 4096;
    static constexpr size_t ALIGNMENT = CACHE_LINE_SIZE;

    static constexpr size_t TINY_LARGE_THRESHOLD = 64;
    static constexpr size_t SMALL_LARGE_THRESHOLD = 256;
    static constexpr size_t LARGE_THRESHOLD = 1024 * 1024;

    static constexpr size_t MAX_CACHED_BLOCKS = 32;
    static constexpr size_t MAX_CACHE_SIZE = 64 * 1024 * 1024;
    static constexpr size_t MIN_CACHE_BLOCK = 4 * 1024;
    static constexpr size_t MAX_CACHE_BLOCK = 16 * 1024 * 1024;
    static constexpr auto MAX_SIZE_RATIO = 1.25;

    static constexpr size_t CACHE_SIZE = 32;
    static constexpr size_t SIZE_CLASSES = 32;
    static constexpr size_t TINY_CLASSES = 8;
    static constexpr size_t MAX_POOLS = 8;

    static constexpr uint64_t SIZE_MASK = 0x0000FFFFFFFFFFFF;
    static constexpr uint64_t CLASS_MASK = 0x00FF000000000000;
    static constexpr uint64_t MMAP_FLAG = 1ULL << 62;
    static constexpr uint64_t COALESCED_FLAG = 1ULL << 61;
    static constexpr uint64_t HEADER_MAGIC = 0xDEADBEEF12345678;
    static constexpr uint64_t MAGIC_MASK = 0xF000000000000000;
    static constexpr uint64_t MAGIC_VALUE = 0xA000000000000000;
    static constexpr uint64_t THREAD_OWNER_MASK = 0xFFFF000000000000;

    struct size_class
    {
        uint16_t size;
        uint16_t slot_size;
        uint16_t blocks;
        uint16_t slack;
    };

    struct thread_cache_t
    {
        struct cached_block
        {
            void* ptr;
            uint8_t size_class;
        };

        struct size_class_cache
        {
            cached_block blocks[CACHE_SIZE];
            size_t count;
        };

        size_class_cache caches[SIZE_CLASSES]{};
        ALWAYS_INLINE void* get(uint8_t size_class) noexcept;
        ALWAYS_INLINE bool put(void *ptr, uint8_t size_class) noexcept;
        ALWAYS_INLINE void clear() noexcept;
    };

    struct bitmap
    {
        static constexpr size_t BITS_PER_WORD = 64;
        static constexpr size_t WORDS_PER_BITMAP = 4;
        alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> words[WORDS_PER_BITMAP]{};

        bitmap() noexcept;
        ALWAYS_INLINE size_t find_free_block(size_t size) noexcept;
        ALWAYS_INLINE void mark_free(size_t index) noexcept;
        ALWAYS_INLINE bool is_completely_free() const noexcept;
    };

    struct alignas(ALIGNMENT) block_header
    {
        // Bit field layout:
        // [63]    - Free flag
        // [62]    - Memory mapped flag
        // [61]    - Coalesced flag
        // [56-48] - Size class
        // [47-0]  - Block size
        uint64_t data;
        uint64_t magic;
        block_header* prev_physical;
        block_header* next_physical;

        ALWAYS_INLINE void init(size_t sz, uint8_t size_class, bool is_free) noexcept;
        ALWAYS_INLINE void encode(size_t size, uint8_t size_class) noexcept;
        [[nodiscard]] ALWAYS_INLINE bool is_valid() const noexcept;
        ALWAYS_INLINE bool set_free(bool is_free) noexcept;
        ALWAYS_INLINE void set_mmapped(bool is_mmap) noexcept;
        [[nodiscard]] ALWAYS_INLINE size_t size() const noexcept;
        [[nodiscard]] ALWAYS_INLINE uint8_t size_class() const noexcept;
        [[nodiscard]] ALWAYS_INLINE bool is_free() const noexcept;
        [[nodiscard]] ALWAYS_INLINE bool is_mmapped() const noexcept;
        [[nodiscard]] ALWAYS_INLINE bool is_aligned() const noexcept;
        ALWAYS_INLINE bool coalesce() noexcept;
        ALWAYS_INLINE void set_coalesced(bool is_coalesced) noexcept;
        [[nodiscard]] ALWAYS_INLINE bool is_coalesced() const noexcept;
    };

    struct alignas(PG_SIZE) pool
    {
        static constexpr size_t MIN_RETURN_SIZE = 64 * 1024;
        static constexpr auto MEM_USAGE_THRESHOLD = 0.2;
        bitmap bitmap;
        uint8_t memory[PG_SIZE - sizeof(bitmap)]{};
        ALWAYS_INLINE void* alloc(const size_class& sc) noexcept;
        ALWAYS_INLINE void free(const void* ptr, const size_class& sc) noexcept;
        ALWAYS_INLINE bool is_completely_free() const noexcept;
        ALWAYS_INLINE static void return_mem() noexcept;
    };

    struct tiny_block_manager
    {
        struct alignas(PG_SIZE) tiny_pool
        {
            bitmap bitmap;
            alignas(ALIGNMENT) uint8_t memory[PG_SIZE - sizeof(bitmap)]{};

            ALWAYS_INLINE void* alloc_tiny(uint8_t size_class) noexcept;
            ALWAYS_INLINE void free_tiny(void* ptr, uint8_t size_class) noexcept;
        };
        tiny_pool pool;

        ALWAYS_INLINE void* alloc_tiny(uint8_t size_class) noexcept;
        ALWAYS_INLINE void free_tiny(void* ptr, uint8_t size_class) noexcept;
    };

    struct pool_manager
    {
        struct pool_entry
        {
            pool* p;
            size_t used_blocks;
        };

        alignas(CACHE_LINE_SIZE) pool_entry pools[SIZE_CLASSES][MAX_POOLS]{};
        size_t pool_count[SIZE_CLASSES]{};
        ALWAYS_INLINE pool* alloc_pool(size_t size_class) noexcept;
        ALWAYS_INLINE void free_pool(const pool* p, size_t size_class) noexcept;

        pool_manager();

        ~pool_manager();
    };

    struct alignas(CACHE_LINE_SIZE) large_block_cache_t
    {
        struct alignas(CACHE_LINE_SIZE) cache_entry
        {
            std::atomic<void*> ptr{nullptr};
            size_t size{0};
            uint64_t last_use{0};
        };

        struct alignas(CACHE_LINE_SIZE) size_bucket
        {
            static constexpr size_t BUCKET_SIZE = 4;
            std::atomic<size_t> count{0};
            cache_entry entries[BUCKET_SIZE];
        };

        static constexpr size_t NUM_BUCKETS = 8; // 4KB to 512KB
        alignas(CACHE_LINE_SIZE) size_bucket buckets[NUM_BUCKETS];
        alignas(CACHE_LINE_SIZE) std::atomic<size_t> total_cached{0};

        ALWAYS_INLINE static uint64_t get_time() noexcept;
        ALWAYS_INLINE static size_t get_bucket_index(size_t size) noexcept;
        ALWAYS_INLINE void *get_cached_block(size_t size) noexcept;
        ALWAYS_INLINE bool cache_block(void *ptr, size_t size) noexcept;
        ALWAYS_INLINE void clear() noexcept;
    };

    extern thread_local thread_cache_t thread_cache_;
    extern thread_local pool_manager* pool_manager_;
    extern thread_local large_block_cache_t* large_block_cache_;
    extern thread_local std::array<tiny_block_manager*, TINY_CLASSES> tiny_pools_;

    ALWAYS_INLINE static void* alloc_tiny(size_t size) noexcept;
    ALWAYS_INLINE static void* alloc_small(size_t size) noexcept;
    ALWAYS_INLINE static void* alloc_medium(size_t size, uint8_t size_class) noexcept;
    ALWAYS_INLINE static void* alloc_large(size_t size) noexcept;
    ALWAYS_INLINE static void cleanup();

    namespace yumina::detail::internal
    {
        void* allocate(size_t size) noexcept;
        void deallocate(void* ptr) noexcept;
        void* reallocate(void* ptr, size_t new_size) noexcept;
        void* callocate(size_t num, size_t size) noexcept;
        void thread_cleanup() noexcept;
        void cleanup() noexcept;
    }
}

#endif