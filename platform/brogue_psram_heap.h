
#ifndef JACK_BROGUE_PSRAM_HEAP_H
#define JACK_BROGUE_PSRAM_HEAP_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void brogue_psram_heap_init(void);
void brogue_psram_heap_init_region(void *memory, size_t bytes);
void *brogue_psram_malloc(size_t size);
void *brogue_psram_calloc(size_t count, size_t size);
void *brogue_psram_realloc(void *pointer, size_t size);
void brogue_psram_free(void *pointer);
size_t brogue_psram_heap_used(void);
size_t brogue_psram_heap_peak(void);
#ifdef __cplusplus
}
#endif
#endif
