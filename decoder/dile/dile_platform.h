#pragma once

#ifndef DECODER_PLATFORM_NAME
#error "DECODER_PLATFORM_NAME Not defined"
#endif

// Coming from https://stackoverflow.com/a/1489985/859190
#define DECODER_DECL_PASTER(x, y) x##_##y
#define DECODER_DECL_EVALUATOR(x, y) DECODER_DECL_PASTER(x, y)
#define DECODER_SYMBOL_NAME(name) DECODER_DECL_EVALUATOR(name, DECODER_PLATFORM_NAME)

int dile_webos_version;