
#include <cstring>
#include "ff.h"

namespace {
struct BrogueDirectory {
    DIR fat;
    char name[256];
    bool open;
};
struct PosixDirent { char d_name[256]; };
BrogueDirectory g_directory{};
PosixDirent g_entry{};
}

extern "C" void *opendir(const char *)
{
    if (g_directory.open) return nullptr;
    if (f_opendir(&g_directory.fat, "/SAVES/BROGUECE") != FR_OK) return nullptr;
    g_directory.open = true;
    return &g_directory;
}

extern "C" PosixDirent *readdir(void *handle)
{
    if (handle != &g_directory || !g_directory.open) return nullptr;
    FILINFO info{};
    for (;;) {
        if (f_readdir(&g_directory.fat, &info) != FR_OK || info.fname[0] == '\0')
            return nullptr;
        if (info.fattrib & AM_DIR) continue;
        strncpy(g_entry.d_name, info.fname, sizeof(g_entry.d_name) - 1);
        g_entry.d_name[sizeof(g_entry.d_name) - 1] = '\0';
        return &g_entry;
    }
}

extern "C" int closedir(void *handle)
{
    if (handle != &g_directory || !g_directory.open) return -1;
    g_directory.open = false;
    return f_closedir(&g_directory.fat) == FR_OK ? 0 : -1;
}
