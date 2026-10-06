
#include <cstdio>
#include <cstring>

#include "ff.h"
#include "brogue_video.h"
#include "hardware/psram.h"
#include "hardware/watchdog.h"
#include "brogue_psram_heap.h"
#include "jack_wave1_runtime.h"
#include "pico/platform/sections.h"
#include "pico/stdlib.h"
#include "brogue_embedded_contract.h"
#include "tusb.h"

enum {
    kReturnKey = 10, kEscapeKey = 27, kDeleteKey = 127, kTabKey = 9,
};

namespace {
// Top 2 MiB of PSRAM are reserved for Brogue's C stack.
constexpr uintptr_t kBrogueStackBottom = 0x11600000u;
constexpr uintptr_t kBrogueStackTop = 0x11800000u;
constexpr size_t kBrogueStackGuardBytes = 64;
extern "C" uint8_t __brogue_psram_bss_start__;
extern "C" uint8_t __brogue_psram_bss_end__;
brogue_video_cell __uninitialized_psram("brogue_cells") g_cells[BROGUE_EMBEDDED_ROWS][BROGUE_EMBEDDED_COLS];
bool g_zoom = false;
unsigned g_view_x = 0;
unsigned g_view_y = 0;
bool g_dirty = true;
uintptr_t g_stack_low_water = kBrogueStackTop;
uint32_t g_brogue_keys_popped = 0;
uint32_t g_brogue_keys_accepted = 0;

bool stack_guard_intact()
{
    const auto *guard = reinterpret_cast<const volatile uint8_t *>(kBrogueStackBottom);
    for (size_t i = 0; i < kBrogueStackGuardBytes; ++i) {
        if (guard[i] != 0xa5) return false;
    }
    return true;
}

void diagnostics_tick()
{
    static uint32_t next_report_us = 0;
    const uint32_t now = time_us_32();
    if (static_cast<int32_t>(now - next_report_us) < 0) return;
    next_report_us = now + 1000000u;
    uintptr_t stack_pointer;
    asm volatile("mov %0, sp" : "=r"(stack_pointer));
    if (stack_pointer < g_stack_low_water) g_stack_low_water = stack_pointer;
    brogue_video_status video{};
    brogue_video_get_status(&video);
    printf("BROGUE_RUNTIME heap=%lu peak=%lu stack_peak=%lu stack_guard=%u usb_reports=%lu key_pop=%lu key_ok=%lu key_q=%u key_drop=%lu zoom=%u video_frames=%lu video_late=%lu line_us=%lu\n",
           static_cast<unsigned long>(brogue_psram_heap_used()),
           static_cast<unsigned long>(brogue_psram_heap_peak()),
           static_cast<unsigned long>(kBrogueStackTop - g_stack_low_water),
           static_cast<unsigned>(stack_guard_intact()),
           static_cast<unsigned long>(jack_wave1_usb_report_count()),
           static_cast<unsigned long>(g_brogue_keys_popped),
           static_cast<unsigned long>(g_brogue_keys_accepted),
           static_cast<unsigned>(jack_wave1_usb_key_queue_depth()),
           static_cast<unsigned long>(jack_wave1_usb_key_queue_drop_count()),
           static_cast<unsigned>(g_zoom),
           static_cast<unsigned long>(video.frames),
           static_cast<unsigned long>(video.late_lines),
           static_cast<unsigned long>(video.peak_line_us));
}

uint16_t channel(short value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    return static_cast<uint16_t>((value * 31 + 50) / 100);
}

uint16_t rgb555(short red, short green, short blue)
{
    return static_cast<uint16_t>((channel(red) << 10) |
                                 (channel(green) << 5) | channel(blue));
}

void plot_glyph(void *, uint32_t glyph, int16_t x, int16_t y,
                int16_t fr, int16_t fg, int16_t fb,
                int16_t br, int16_t bg, int16_t bb)
{
    if (x < 0 || x >= BROGUE_EMBEDDED_COLS || y < 0 || y >= BROGUE_EMBEDDED_ROWS) return;
    brogue_video_cell &cell = g_cells[y][x];
    cell = {brogue_video_ascii_glyph(brogue_embedded_glyph_unicode(glyph)),
            rgb555(fr, fg, fb), rgb555(br, bg, bb)};
    g_dirty = true;
}

void present(void *)
{
    if (!g_dirty) return;
    while (!brogue_video_submit(&g_cells[0][0], g_zoom, g_view_x, g_view_y)) {
        jack_wave1_usb_service();
        diagnostics_tick();
        sleep_ms(1);
    }
    g_dirty = false;
}

void adjust_view(int dx, int dy)
{
    int x = static_cast<int>(g_view_x) + dx;
    int y = static_cast<int>(g_view_y) + dy;
    if (x < 0) x = 0;
    if (x > BROGUE_EMBEDDED_COLS - 40) x = BROGUE_EMBEDDED_COLS - 40;
    if (y < 0) y = 0;
    if (y > BROGUE_EMBEDDED_ROWS - 30) y = BROGUE_EMBEDDED_ROWS - 30;
    g_view_x = static_cast<unsigned>(x);
    g_view_y = static_cast<unsigned>(y);
    g_dirty = true;
    present(nullptr);
}

void toggle_zoom()
{
    g_zoom = !g_zoom;
    if (g_zoom) {

        g_view_x = (BROGUE_EMBEDDED_COLS - 40) / 2;
        g_view_y = (BROGUE_EMBEDDED_ROWS - 30) / 2;
    }
    g_dirty = true;
    present(nullptr);
}

void delay_ms(void *, uint16_t milliseconds)
{
    while (milliseconds--) {
        jack_wave1_usb_service();
        diagnostics_tick();
        sleep_ms(1);
    }
}

int ascii_key(uint8_t code, bool shift)
{
    if (code >= HID_KEY_A && code <= HID_KEY_Z)
        return (shift ? 'A' : 'a') + code - HID_KEY_A;
    if (code >= HID_KEY_1 && code <= HID_KEY_9) {
        static const char shifted[] = "!@#$%^&*(";
        return shift ? shifted[code - HID_KEY_1] : '1' + code - HID_KEY_1;
    }
    if (code == HID_KEY_0) return shift ? ')' : '0';
    switch (code) {
        case HID_KEY_SPACE: return ' ';
        case HID_KEY_ENTER: return kReturnKey;
        case HID_KEY_ESCAPE: return kEscapeKey;
        case HID_KEY_BACKSPACE: return kDeleteKey;
        case HID_KEY_TAB: return kTabKey;
        case HID_KEY_ARROW_UP: return 'k';
        case HID_KEY_ARROW_DOWN: return 'j';
        case HID_KEY_ARROW_LEFT: return 'h';
        case HID_KEY_ARROW_RIGHT: return 'l';
        case HID_KEY_MINUS: return shift ? '_' : '-';
        case HID_KEY_EQUAL: return shift ? '+' : '=';
        case HID_KEY_BRACKET_LEFT: return shift ? '{' : '[';
        case HID_KEY_BRACKET_RIGHT: return shift ? '}' : ']';
        case HID_KEY_BACKSLASH: return shift ? '|' : '\\';
        case HID_KEY_SEMICOLON: return shift ? ':' : ';';
        case HID_KEY_APOSTROPHE: return shift ? '"' : '\'';
        case HID_KEY_GRAVE: return shift ? '~' : '`';
        case HID_KEY_COMMA: return shift ? '<' : ',';
        case HID_KEY_PERIOD: return shift ? '>' : '.';
        case HID_KEY_SLASH: return shift ? '?' : '/';
        default: return 0;
    }
}

bool read_key(void *, bool wait, brogue_embedded_key_event *event)
{
    for (;;) {
        jack_wave1_usb_service();
        diagnostics_tick();
        jack_wave1_key_event_t key{};
        if (jack_wave1_usb_pop_key(&key)) {
            ++g_brogue_keys_popped;
            if (key.keycode == HID_KEY_F1) { toggle_zoom(); continue; }
            if (g_zoom && key.keycode == HID_KEY_F2) { adjust_view(-10, 0); continue; }
            if (g_zoom && key.keycode == HID_KEY_F3) { adjust_view(10, 0); continue; }
            if (g_zoom && key.keycode == HID_KEY_F4) { adjust_view(0, -2); continue; }
            if (g_zoom && key.keycode == HID_KEY_F5) { adjust_view(0, 2); continue; }
            const bool shift = (key.modifier & (KEYBOARD_MODIFIER_LEFTSHIFT |
                                                 KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
            const int value = ascii_key(key.keycode, shift);
            if (value != 0) {
                ++g_brogue_keys_accepted;
                printf("BROGUE_KEY raw=%u modifier=%02x ascii=%d\n",
                       static_cast<unsigned>(key.keycode),
                       static_cast<unsigned>(key.modifier), value);
                *event = {value,
                          (key.modifier & (KEYBOARD_MODIFIER_LEFTCTRL |
                                           KEYBOARD_MODIFIER_RIGHTCTRL)) != 0,
                          shift};
                return true;
            }
        }
        if (!wait) return false;
        sleep_ms(1);
    }
}

bool modifier_held(void *, int modifier)
{
    const uint8_t mask = modifier == 0 ?
        KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT :
        KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL;
    return (jack_wave1_usb_current_modifiers() & mask) != 0;
}

const brogue_embedded_backend kBackend = {
    nullptr, plot_glyph, read_key, delay_ms, present, modifier_held
};
}

extern "C" __attribute__((naked, noinline)) int
brogue_embedded_psram_stack_call(int (*)(void), uintptr_t)
{
    asm volatile(
        "mov r2, sp\n"
        "mov r3, lr\n"
        "mov sp, r1\n"
        "push {r2, r3}\n"
        "blx r0\n"
        "pop {r2, r3}\n"
        "mov sp, r2\n"
        "bx r3\n");
}

int main()
{
    if (!jack_wave1_init("Brogue CE")) for (;;) tight_loop_contents();
    if (!psram_is_available()) {
        jack_wave1_show_error("BROGUE CE", "PSRAM unavailable");
        for (;;) tight_loop_contents();
    }
    memset(&__brogue_psram_bss_start__, 0,
           &__brogue_psram_bss_end__ - &__brogue_psram_bss_start__);
    brogue_psram_heap_init();
    memset(reinterpret_cast<void *>(kBrogueStackBottom), 0xa5, kBrogueStackGuardBytes);
    g_stack_low_water = kBrogueStackTop;
    if (!jack_wave1_mount_sd()) {
        jack_wave1_show_error("BROGUE CE", "SD mount failed");
        for (;;) tight_loop_contents();
    }
    (void)f_mkdir("/SAVES");
    (void)f_mkdir("/SAVES/BROGUECE");
    memset(g_cells, 0, sizeof(g_cells));
    jack_wave1_clear(0);
    brogue_video_init();
    brogue_embedded_install_backend(&kBackend);
    printf("BROGUE_BOOT build=0.1.1-fruit-jam-source-candidate grid=100x34 video=640x480 cell=6x14 heap=%lu sd=ok\n",
           static_cast<unsigned long>(brogue_psram_heap_used()));
    printf("BROGUE_STAGE stack_switch top=0x%08lx size=%lu\n",
           static_cast<unsigned long>(kBrogueStackTop),
           static_cast<unsigned long>(kBrogueStackTop - kBrogueStackBottom));
    const int result = brogue_embedded_run();
    printf("BROGUE_EXIT status=%d\n", result);
    sleep_ms(100);
    watchdog_reboot(0, 0, 100);
    for (;;) tight_loop_contents();
}
