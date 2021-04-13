#pragma once

#include <stdbool.h>

#define PLATFORM_HDR_NONE 0
#define PLATFORM_HDR_SUPPORTED 1
#define PLATFORM_HDR_ALWAYS 2

#ifdef DECODER_PLATFORM_NAME
// Coming from https://stackoverflow.com/a/1489985/859190
#define DECODER_DECL_PASTER(x, y) x##_##y
#define DECODER_DECL_EVALUATOR(x, y) DECODER_DECL_PASTER(x, y)
#define DECODER_SYMBOL_NAME(name) DECODER_DECL_EVALUATOR(name, DECODER_PLATFORM_NAME)
#endif

#ifndef DECODER_EXPORTED
#define DECODER_EXPORTED __attribute__((visibility("default")))
#endif

typedef struct PLATFORM_INFO
{
    bool valid;
    unsigned int vrank;
    unsigned int arank;
    bool vindependent;
    bool aindependent;
    bool hevc;
    int hdr;
    int colorSpace;
    int colorRange;
    int maxBitrate;
} * PPLATFORM_INFO, PLATFORM_INFO;