#pragma once

#if TARGET_WEBOS
#define FONT_FAMILY "Museo Sans"
#elif OS_WINDOWS
#define FONT_FAMILY "Segoe UI"
#else
#define FONT_FAMILY "Dejavu Sans"
#endif
#define FONT_FAMILY_FALLBACK "sans-serif"