#pragma once

#include <stdbool.h>

bool HLunaServiceCallSync(const char *uri, const char *payload, bool public, char **output);