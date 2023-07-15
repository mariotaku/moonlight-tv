#pragma once

#define INI_FULL_MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, n) == 0)
#define INI_NAME_MATCH(n) (strcmp(name, n) == 0)
#define INI_IS_TRUE(v) (strcmp("true", v) == 0)
