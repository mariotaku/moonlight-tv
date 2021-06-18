cmake_minimum_required (VERSION 3.6)

if (TARGET_WEBOS)
    add_subdirectory(ndlaud)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        add_subdirectory(ndlaud-webos5)
    endif()
else()
    add_subdirectory(alsa)
endif()

add_subdirectory(pulse)