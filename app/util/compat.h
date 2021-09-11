#pragma once

#if __WIN32
#define MKDIR(path, perm) mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path, perm) mkdir(path, perm)
#endif