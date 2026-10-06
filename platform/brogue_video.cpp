
#include "brogue_video.h"
#include "brogue_font_6x8.h"
#include "font_8x8.h"
#include <atomic>
#include <cstring>

#ifdef BROGUE_VIDEO_HOST_TEST
#define VIDEO_RAM(name) name
#else
#include "pico.h"
#define VIDEO_RAM(name) __not_in_flash_func(name)
#endif

namespace {
struct Snapshot {
    brogue_video_cell cells[BROGUE_EMBEDDED_ROWS][BROGUE_EMBEDDED_COLS];
    unsigned view_x, view_y;
    bool zoom;
};

Snapshot g_snapshots[2];
std::atomic<unsigned> g_front{0};
std::atomic<int> g_pending{-1};
bool g_enabled = false;
uint8_t g_font6[95][8];
uint8_t g_font8[8 * 95];
std::atomic<uint32_t> g_frames{0}, g_late_lines{0}, g_peak_line_us{0};
}

extern "C" void brogue_video_init(void)
{

    memcpy(g_font6, brogue_font_6x8, sizeof(g_font6));
    memcpy(g_font8, font_8x8, sizeof(g_font8));
}

extern "C" bool brogue_video_submit(const brogue_video_cell *cells, bool zoom,
                                    unsigned view_x, unsigned view_y)
{
    if (!cells || g_pending.load(std::memory_order_acquire) >= 0) return false;
    const unsigned back = g_front.load(std::memory_order_relaxed) ^ 1u;
    Snapshot &snapshot = g_snapshots[back];
    memcpy(snapshot.cells, cells, sizeof(snapshot.cells));
    snapshot.zoom = zoom;
    snapshot.view_x = view_x <= 60 ? view_x : 60;
    snapshot.view_y = view_y <= 4 ? view_y : 4;
    g_pending.store(static_cast<int>(back), std::memory_order_release);
    return true;
}

extern "C" bool VIDEO_RAM(brogue_video_render_scanline)(uint16_t *pixels, unsigned line)
{
    if (line >= BROGUE_VIDEO_HEIGHT) return false;
    if (line == 0) {
        const int pending = g_pending.load(std::memory_order_acquire);
        if (pending >= 0) {
            g_front.store(static_cast<unsigned>(pending), std::memory_order_relaxed);
            g_enabled = true;

            // Release only after core 1 owns the new front buffer.
            g_pending.store(-1, std::memory_order_release);
        }
        g_frames.store(g_frames.load(std::memory_order_relaxed) + 1,
                       std::memory_order_relaxed);
    }
    if (!g_enabled) return false;
    const Snapshot &snapshot = g_snapshots[g_front.load(std::memory_order_relaxed)];
    if (snapshot.zoom) {
        const brogue_video_cell *cells = snapshot.cells[line / 16 + snapshot.view_y]
                                                        + snapshot.view_x;
        const unsigned row = (line % 16) / 2;
        for (unsigned x = 0; x < 40; ++x) {
            const brogue_video_cell &cell = cells[x];
            const unsigned glyph = cell.glyph >= 32 && cell.glyph <= 126 ? cell.glyph - 32 : '?' - 32;
            const uint8_t bits = g_font8[row * 95 + glyph];
            for (unsigned bit = 0; bit < 8; ++bit) {
                const uint16_t color = bits & (1u << bit) ? cell.foreground : cell.background;
                *pixels++ = color;
                *pixels++ = color;
            }
        }
        return true;
    }

    if (line < 2 || line >= 478) {
        for (unsigned x = 0; x < BROGUE_VIDEO_WIDTH; ++x) pixels[x] = 0;
        return true;
    }
    const unsigned cell_y = (line - 2) / 14;
    const unsigned row = ((line - 2) % 14) * 8 / 14;
    const brogue_video_cell *cells = snapshot.cells[cell_y];
    for (unsigned x = 0; x < 20; ++x) *pixels++ = 0;
    for (unsigned x = 0; x < BROGUE_EMBEDDED_COLS; ++x) {
        const brogue_video_cell &cell = cells[x];
        const unsigned glyph = cell.glyph >= 32 && cell.glyph <= 126 ? cell.glyph - 32 : '?' - 32;
        const uint8_t bits = g_font6[glyph][row];
        const uint16_t fg = cell.foreground, bg = cell.background;
        *pixels++ = bits & 1u ? fg : bg;
        *pixels++ = bits & 2u ? fg : bg;
        *pixels++ = bits & 4u ? fg : bg;
        *pixels++ = bits & 8u ? fg : bg;
        *pixels++ = bits & 16u ? fg : bg;
        *pixels++ = bits & 32u ? fg : bg;
    }
    for (unsigned x = 0; x < 20; ++x) *pixels++ = 0;
    return true;
}

extern "C" void VIDEO_RAM(brogue_video_record_scanline)(uint32_t elapsed_us, bool late)
{
    if (late) g_late_lines.store(g_late_lines.load(std::memory_order_relaxed) + 1,
                                std::memory_order_relaxed);
    if (elapsed_us > g_peak_line_us.load(std::memory_order_relaxed))
        g_peak_line_us.store(elapsed_us, std::memory_order_relaxed);
}

extern "C" void brogue_video_get_status(brogue_video_status *status)
{
    *status = {g_frames.load(std::memory_order_relaxed),
               g_late_lines.load(std::memory_order_relaxed),
               g_peak_line_us.load(std::memory_order_relaxed)};
}

extern "C" uint8_t brogue_video_ascii_glyph(uint32_t unicode)
{
    if (unicode >= 32 && unicode <= 126) return static_cast<uint8_t>(unicode);
    switch (unicode) {
        case 0x2191: return '^'; case 0x2193: return 'v';
        case 0x2190: return '<'; case 0x2192: return '>';
        case 0x00b7: return '.'; case 0x2237: return ':';
        case 0x25c7: case 0x22cf: return '^';
        case 0x2648: return '"'; case 0x2640: return '&';
        case 0x26aa: return 'o'; case 0x26b2: return '+';
        case 0x29f3: return '*'; case 0x29f2: return 'x';
        case 0x03a9: return 'O'; case 0x03df: return '*';
        case 0x00df: return 'S'; case 0x00da: return 'U';
        case 0x00a4: return '$'; case 0x1f780: return '<';
        case 0x25cf: return 'o'; case 0x266a: return '*';
        default: return '?';
    }
}
