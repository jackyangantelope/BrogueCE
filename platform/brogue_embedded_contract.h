
#ifndef JACK_BROGUE_EMBEDDED_CONTRACT_H
#define JACK_BROGUE_EMBEDDED_CONTRACT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BROGUE_EMBEDDED_COLS 100
#define BROGUE_EMBEDDED_ROWS 34

typedef struct brogue_embedded_key_event {
    int32_t key;
    bool control;
    bool shift;
} brogue_embedded_key_event;

typedef struct brogue_embedded_backend {
    void *context;
    void (*plot_glyph)(void *context, uint32_t glyph, int16_t x, int16_t y,
                       int16_t fore_red, int16_t fore_green, int16_t fore_blue,
                       int16_t back_red, int16_t back_green, int16_t back_blue);
    bool (*read_key)(void *context, bool wait, brogue_embedded_key_event *event);
    void (*delay_ms)(void *context, uint16_t milliseconds);
    void (*present)(void *context);
    bool (*modifier_held)(void *context, int modifier);
} brogue_embedded_backend;

struct brogueConsole;
extern struct brogueConsole brogueEmbeddedConsole;

void brogue_embedded_install_backend(const brogue_embedded_backend *backend);
int brogue_embedded_run(void);
unsigned int brogue_embedded_glyph_unicode(uint32_t glyph);

#ifdef __cplusplus
}
#endif

#endif
