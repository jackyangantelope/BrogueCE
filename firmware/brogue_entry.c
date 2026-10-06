
#include "platform.h"
#include "brogue_embedded_contract.h"
#include <stdint.h>
#include <stdio.h>

extern int brogue_embedded_psram_stack_call(int (*entry)(void), uintptr_t stack_top);

static int brogue_embedded_run_game(void)
{
    printf("BROGUE_STAGE psram_stack_active\n");
    return rogueMain();
}

struct brogueConsole currentConsole;
char dataDirectory[BROGUE_FILENAME_MAX] = "/SAVES/BROGUECE";
boolean serverMode = false;
boolean nonInteractivePlayback = false;
boolean hasGraphics = false;
enum graphicsModes graphicsMode = TEXT_GRAPHICS;
boolean isCsvFormat = false;

boolean tryParseUint64(char *text, uint64_t *number)
{
    if (!text || !number || !text[0] || (text[0] == '0' && text[1])) return false;
    uint64_t value = 0;
    for (const char *digit = text; *digit; ++digit) {
        if (*digit < '0' || *digit > '9') return false;
        const unsigned next = (unsigned)(*digit - '0');
        if (value > (UINT64_MAX - next) / 10) return false;
        value = value * 10 + next;
    }
    *number = value;
    return true;
}

unsigned int brogue_embedded_glyph_unicode(uint32_t glyph)
{
    return glyphToUnicode((enum displayGlyph)glyph);
}

int brogue_embedded_run(void)
{
    currentConsole = brogueEmbeddedConsole;
    rogue.nextGame = NG_NOTHING;
    rogue.nextGamePath[0] = '\0';
    rogue.nextGameSeed = 0;
    rogue.mode = GAME_MODE_NORMAL;
    return brogue_embedded_psram_stack_call(brogue_embedded_run_game, 0x11800000u);
}
