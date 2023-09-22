#pragma once

#if __WIN32
#define MKDIR(path, perm) mkdir(path)
#define PATH_MAX 260
#else
#include <sys/stat.h>
#define MKDIR(path, perm) mkdir(path, perm)
#define PATH_MAX 4096
#endif