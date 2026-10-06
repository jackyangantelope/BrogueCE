
#ifndef BROGUE_VIDEO_H
#define BROGUE_VIDEO_H

#include <stdbool.h>
#include <stdint.h>
#include "brogue_embedded_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { BROGUE_VIDEO_WIDTH = 640, BROGUE_VIDEO_HEIGHT = 480 };
typedef struct brogue_video_cell {
    uint8_t glyph;
    uint16_t foreground;
    uint16_t background;
} brogue_video_cell;

typedef struct brogue_video_status {
    uint32_t frames;
    uint32_t late_lines;
    uint32_t peak_line_us;
} brogue_video_status;

void brogue_video_init(void);
uint8_t brogue_video_ascii_glyph(uint32_t unicode);

bool brogue_video_submit(const brogue_video_cell *cells, bool zoom,
                        unsigned view_x, unsigned view_y);

bool brogue_video_render_scanline(uint16_t *pixels, unsigned line);
void brogue_video_record_scanline(uint32_t elapsed_us, bool late);
void brogue_video_get_status(brogue_video_status *status);

#ifdef __cplusplus
}
#endif
#endif
