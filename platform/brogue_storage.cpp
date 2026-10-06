
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>

#include "ff.h"
#include "hardware/uart.h"

namespace {
constexpr unsigned kFileSlots = 6;
struct FileSlot { FIL file; bool used; };
FileSlot g_files[kFileSlots]{};

bool resolve_path(const char *path, char *resolved, size_t capacity)
{
    if (!path || !*path) return false;
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    if (!*name || strchr(name, '\\') || strstr(name, "..")) return false;
    const int count = snprintf(resolved, capacity, "/SAVES/BROGUECE/%s", name);
    return count > 0 && static_cast<size_t>(count) < capacity;
}

BYTE fatfs_mode(int flags)
{
    BYTE mode = (flags & O_RDWR) ? FA_READ | FA_WRITE :
                (flags & O_WRONLY) ? FA_WRITE : FA_READ;
    if (flags & O_CREAT) mode |= (flags & O_TRUNC) ? FA_CREATE_ALWAYS : FA_OPEN_ALWAYS;
    return mode;
}
}

extern "C" int _open(const char *path, int flags, ...)
{
    char resolved[256];
    if (!resolve_path(path, resolved, sizeof(resolved))) { errno = EINVAL; return -1; }
    for (unsigned i = 0; i < kFileSlots; ++i) {
        if (g_files[i].used) continue;
        if (f_open(&g_files[i].file, resolved, fatfs_mode(flags)) != FR_OK) continue;
        g_files[i].used = true;
        if (flags & O_APPEND) (void)f_lseek(&g_files[i].file, f_size(&g_files[i].file));
        return static_cast<int>(i + 3);
    }
    errno = ENOENT;
    return -1;
}

extern "C" int _close(int fd)
{
    if (fd < 3 || fd >= static_cast<int>(3 + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    const FRESULT result = f_close(&g_files[fd - 3].file);
    g_files[fd - 3].used = false;
    return result == FR_OK ? 0 : -1;
}

extern "C" int _read(int fd, char *buffer, int count)
{
    if (fd < 3 || fd >= static_cast<int>(3 + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    UINT read = 0;
    return f_read(&g_files[fd - 3].file, buffer, count, &read) == FR_OK ?
        static_cast<int>(read) : -1;
}

extern "C" int _write(int fd, const char *buffer, int count)
{
    if (fd == 1 || fd == 2) {
        if (count > 0) uart_write_blocking(uart0,
            reinterpret_cast<const uint8_t *>(buffer), static_cast<size_t>(count));
        return count;
    }
    if (fd < 3 || fd >= static_cast<int>(3 + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    UINT written = 0;
    return f_write(&g_files[fd - 3].file, buffer, count, &written) == FR_OK ?
        static_cast<int>(written) : -1;
}

extern "C" int _lseek(int fd, int offset, int whence)
{
    if (fd < 3 || fd >= static_cast<int>(3 + kFileSlots) || !g_files[fd - 3].used) {
        errno = EBADF; return -1;
    }
    FIL &file = g_files[fd - 3].file;
    const int64_t base = whence == SEEK_SET ? 0 :
        whence == SEEK_CUR ? static_cast<int64_t>(f_tell(&file)) :
        whence == SEEK_END ? static_cast<int64_t>(f_size(&file)) : -1;
    const int64_t position = base + offset;
    if (base < 0 || position < 0 || position > INT32_MAX ||
        f_lseek(&file, static_cast<FSIZE_t>(position)) != FR_OK) {
        errno = EINVAL; return -1;
    }
    return static_cast<int>(f_tell(&file));
}

extern "C" int _fstat(int fd, struct stat *status)
{
    memset(status, 0, sizeof(*status));
    status->st_mode = fd <= 2 ? S_IFCHR : S_IFREG;
    if (fd >= 3 && fd < static_cast<int>(3 + kFileSlots) && g_files[fd - 3].used)
        status->st_size = static_cast<off_t>(f_size(&g_files[fd - 3].file));
    return 0;
}

extern "C" int _stat(const char *path, struct stat *status)
{
    char resolved[256];
    FILINFO info{};
    if (!resolve_path(path, resolved, sizeof(resolved)) || f_stat(resolved, &info) != FR_OK) {
        errno = ENOENT; return -1;
    }
    memset(status, 0, sizeof(*status));
    status->st_mode = (info.fattrib & AM_DIR) ? S_IFDIR : S_IFREG;
    status->st_size = static_cast<off_t>(info.fsize);
    return 0;
}

extern "C" int _unlink(const char *path)
{
    char resolved[256];
    if (!resolve_path(path, resolved, sizeof(resolved)) || f_unlink(resolved) != FR_OK) {
        errno = ENOENT; return -1;
    }
    return 0;
}

extern "C" int rename(const char *old_path, const char *new_path)
{
    char old_resolved[256], new_resolved[256];
    if (!resolve_path(old_path, old_resolved, sizeof(old_resolved)) ||
        !resolve_path(new_path, new_resolved, sizeof(new_resolved)) ||
        f_rename(old_resolved, new_resolved) != FR_OK) {
        errno = EIO; return -1;
    }
    return 0;
}

extern "C" int remove(const char *path) { return _unlink(path); }
extern "C" int _isatty(int fd) { return fd >= 0 && fd <= 2; }
