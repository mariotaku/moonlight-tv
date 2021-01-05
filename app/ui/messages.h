#pragma once

static const char *MSG_GS_ERRNO[11] = {
    "OK",                                                           // GS_OK
    "GS_FAILED",                                                    // GS_FAILED
    "Out of memory",                                                // GS_OUT_OF_MEMORY
    "Invalid data received from server",                            // GS_INVALID
    "GS_WRONG_STATE",                                               // GS_WRONG_STATE
    "I/O Error",                                                    // GS_IO_ERROR
    "Server doesn't support 4K",                                    // GS_NOT_SUPPORTED_4K
    "Unsupported version",                                          // GS_UNSUPPORTED_VERSION
    "Server doesn't support specified resolution and/or framerate", // GS_NOT_SUPPORTED_MODE
    "Gamestream error",                                             // GS_ERROR
    "SOPS isn't supported for the specified resolution",            // GS_NOT_SUPPORTED_SOPS_RESOLUTION
};
