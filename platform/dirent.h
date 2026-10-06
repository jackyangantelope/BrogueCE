
#ifndef JACK_BROGUE_DIRENT_H
#define JACK_BROGUE_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct brogue_directory DIR;
struct dirent { char d_name[256]; };
DIR *opendir(const char *path);
struct dirent *readdir(DIR *directory);
int closedir(DIR *directory);

#ifdef __cplusplus
}
#endif
#endif
