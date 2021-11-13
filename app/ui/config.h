#pragma once

#if TARGET_WEBOS
#define FONT_FAMILY "Museo Sans"
#elif TARGET_LINUX
#define FONT_FAMILY "Dejavu Sans"
#else
#define FONT_FAMILY "Segoe UI"
#endif
#define FONT_FAMILY_FALLBACK "sans-serif"