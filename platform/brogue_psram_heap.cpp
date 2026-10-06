
#include "brogue_psram_heap.h"
#include <cstdint>
#include <cstring>
#ifndef BROGUE_HEAP_HOST_TEST
#include "hardware/psram.h"
#endif

#ifndef BROGUE_HEAP_HOST_TEST
extern "C" char __psram_end__;
#endif
namespace {

// Heap must stop below the reserved stack region.
constexpr uintptr_t kPsramEnd = 0x11600000u;
constexpr uint32_t kMagic = 0x42525053u;
struct alignas(8) Block {
    size_t size;
    Block *next;
    uint32_t magic;
    uint32_t free;
};
Block *g_first;
uintptr_t g_arena_start;
uintptr_t g_arena_end;
size_t g_used;
size_t g_peak;
size_t align8(size_t size) { return (size + 7u) & ~size_t(7u); }
bool adjacent(const Block *first, const Block *second)
{
    return reinterpret_cast<uintptr_t>(first + 1) + first->size ==
        reinterpret_cast<uintptr_t>(second);
}
void merge_next(Block *block)
{
    if (block->next && block->next->free && adjacent(block, block->next)) {
        block->size += sizeof(Block) + block->next->size;
        block->next = block->next->next;
    }
}
Block *find_block(void *pointer)
{
    const uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
    if (!g_first || address < g_arena_start + sizeof(Block) ||
        address >= g_arena_end || (address & 7u)) return nullptr;
    Block *block = reinterpret_cast<Block *>(pointer) - 1;
    return block->magic == kMagic && !block->free ? block : nullptr;
}
}

#ifndef BROGUE_HEAP_HOST_TEST
extern "C" void brogue_psram_heap_init(void)
{
    const uintptr_t start = reinterpret_cast<uintptr_t>(&__psram_end__);
    if (!psram_is_available() || start >= kPsramEnd) {
        brogue_psram_heap_init_region(nullptr, 0);
        return;
    }
    brogue_psram_heap_init_region(reinterpret_cast<void *>(start), kPsramEnd - start);
}
#endif

extern "C" void brogue_psram_heap_init_region(void *memory, size_t bytes)
{
    const uintptr_t raw_start = reinterpret_cast<uintptr_t>(memory);
    const uintptr_t end = raw_start + bytes;
    const uintptr_t start = align8(raw_start);
    g_first = nullptr;
    g_arena_start = g_arena_end = 0;
    g_used = g_peak = 0;
    if (!memory || end < raw_start || start + sizeof(Block) >= end) return;
    g_arena_start = start;
    g_arena_end = end;
    g_first = reinterpret_cast<Block *>(start);
    *g_first = {end - start - sizeof(Block), nullptr, kMagic, 1};
}

extern "C" void *brogue_psram_malloc(size_t requested)
{
    if (!g_first || !requested || requested > g_arena_end - g_arena_start - sizeof(Block))
        return nullptr;
    const size_t size = align8(requested);
    for (Block *block = g_first; block; block = block->next) {
        if (!block->free || block->size < size) continue;
        if (block->size >= size + sizeof(Block) + 16) {
            auto *remainder = reinterpret_cast<Block *>(
                reinterpret_cast<uintptr_t>(block + 1) + size);
            *remainder = {block->size - size - sizeof(Block), block->next, kMagic, 1};
            block->next = remainder;
            block->size = size;
        }
        block->free = 0;
        g_used += block->size;
        if (g_used > g_peak) g_peak = g_used;
        return block + 1;
    }
    return nullptr;
}

extern "C" void *brogue_psram_calloc(size_t count, size_t size)
{
    if (count && size > SIZE_MAX / count) return nullptr;
    const size_t bytes = count * size;
    void *memory = brogue_psram_malloc(bytes);
    if (memory) memset(memory, 0, bytes);
    return memory;
}

extern "C" void brogue_psram_free(void *pointer)
{
    if (!pointer) return;
    Block *block = find_block(pointer);
    if (!block || block->free) return;
    block->free = 1;
    g_used -= block->size;
    merge_next(block);
    for (Block *previous = g_first; previous && previous->next;
         previous = previous->next) {
        if (previous->next == block) {
            if (previous->free) merge_next(previous);
            break;
        }
    }
}

extern "C" void *brogue_psram_realloc(void *pointer, size_t size)
{
    if (!pointer) return brogue_psram_malloc(size);
    if (!size) { brogue_psram_free(pointer); return nullptr; }
    Block *block = find_block(pointer);
    if (!block || block->free) return nullptr;
    if (size <= block->size) return pointer;
    void *replacement = brogue_psram_malloc(size);
    if (!replacement) return nullptr;
    memcpy(replacement, pointer, block->size);
    brogue_psram_free(pointer);
    return replacement;
}

extern "C" size_t brogue_psram_heap_used(void) { return g_used; }
extern "C" size_t brogue_psram_heap_peak(void) { return g_peak; }
