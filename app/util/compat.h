#pragma once

#if __WIN32
#define MKDIR(path, perm) mkdir(path)
#else
#define MKDIR(path, perm) mkdir(path, perm)
#endif