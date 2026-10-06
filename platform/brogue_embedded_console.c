
#include <stddef.h>
#include <stdio.h>

#include "platform.h"
#include "brogue_embedded_contract.h"

_Static_assert(COLS == BROGUE_EMBEDDED_COLS, "Brogue column contract changed");
_Static_assert(ROWS == BROGUE_EMBEDDED_ROWS, "Brogue row contract changed");

static const brogue_embedded_backend *g_backend;
static brogue_embedded_key_event g_pending_key;
static boolean g_has_pending_key;

void brogue_embedded_install_backend(const brogue_embedded_backend *backend)
{
    g_backend = backend;
    g_has_pending_key = false;
}

static boolean fetch_key(boolean wait)
{
    if (g_has_pending_key) {
        return true;
    }
    if (g_backend == NULL || g_backend->read_key == NULL) {
        return false;
    }
    g_has_pending_key = g_backend->read_key(g_backend->context, wait != false, &g_pending_key);
    return g_has_pending_key;
}

static void embedded_game_loop(void)
{
    (void)rogueMain();
}

static boolean embedded_pause(short milliseconds, PauseBehavior behavior)
{
    (void)behavior;
    if (g_backend != NULL && g_backend->present != NULL) {
        g_backend->present(g_backend->context);
    }
    if (fetch_key(false)) {
        return true;
    }
    if (milliseconds > 0 && g_backend != NULL && g_backend->delay_ms != NULL) {
        g_backend->delay_ms(g_backend->context, (uint16_t)milliseconds);
    }
    return fetch_key(false);
}

static void embedded_next_event(rogueEvent *return_event, boolean text_input, boolean colors_dance)
{
    (void)text_input;
    (void)colors_dance;
    if (g_backend != NULL && g_backend->present != NULL) {
        g_backend->present(g_backend->context);
    }
    while (!fetch_key(true)) {
        if (g_backend != NULL && g_backend->delay_ms != NULL) {
            g_backend->delay_ms(g_backend->context, 1);
        }
    }
    return_event->eventType = KEYSTROKE;
    return_event->param1 = g_pending_key.key;
    return_event->param2 = 0;
    return_event->controlKey = g_pending_key.control;
    return_event->shiftKey = g_pending_key.shift;
    printf("BROGUE_EVENT type=%d key=%ld ctrl=%d shift=%d\n",
           return_event->eventType, (long)return_event->param1,
           return_event->controlKey, return_event->shiftKey);
    g_has_pending_key = false;
}

static void embedded_plot_char(enum displayGlyph glyph, short x, short y,
                               short fore_red, short fore_green, short fore_blue,
                               short back_red, short back_green, short back_blue)
{
    if (g_backend != NULL && g_backend->plot_glyph != NULL) {
        g_backend->plot_glyph(g_backend->context, (uint32_t)glyph, x, y,
                              fore_red, fore_green, fore_blue,
                              back_red, back_green, back_blue);
    }
}

static boolean embedded_modifier_held(int modifier)
{
    return g_backend != NULL && g_backend->modifier_held != NULL
               ? g_backend->modifier_held(g_backend->context, modifier)
               : false;
}

static enum graphicsModes embedded_set_graphics_mode(enum graphicsModes mode)
{
    (void)mode;
    return TEXT_GRAPHICS;
}

struct brogueConsole brogueEmbeddedConsole = {
    embedded_game_loop,
    embedded_pause,
    embedded_next_event,
    embedded_plot_char,
    NULL,
    embedded_modifier_held,
    NULL,
    NULL,
    embedded_set_graphics_mode
};
