cmake_minimum_required (VERSION 3.6)

if (TARGET_WEBOS)
    add_subdirectory(ndlaud)
    add_subdirectory(pulse)
else()
    add_subdirectory(alsa)
    add_subdirectory(pulse)
endif()