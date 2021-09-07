set(MICRODNS_DEP '')

include(CheckIncludeFile)
include(CheckFunctionExists)

add_library(microdns
        ${CMAKE_SOURCE_DIR}/third_party/libmicrodns/src/mdns.c
        ${CMAKE_SOURCE_DIR}/third_party/libmicrodns/src/rr.c
        ${CMAKE_SOURCE_DIR}/third_party/libmicrodns/compat/compat.c
        ${CMAKE_SOURCE_DIR}/third_party/libmicrodns/compat/inet.c
        ${CMAKE_SOURCE_DIR}/third_party/libmicrodns/compat/poll.c
        )
target_include_directories(microdns SYSTEM PUBLIC third_party/libmicrodns/include)
target_include_directories(microdns PRIVATE third_party/libmicrodns/compat)

check_function_exists(inet_ntop HAVE_INET_NTOP)
if (HAVE_INET_NTOP)
    target_compile_definitions(microdns PRIVATE HAVE_INET_NTOP=1)
endif ()

check_function_exists(poll HAVE_POLL)
if (HAVE_POLL)
    target_compile_definitions(microdns PRIVATE HAVE_POLL=1)
endif ()

check_function_exists(getifaddrs HAVE_GETIFADDRS)
if (HAVE_GETIFADDRS)
    target_compile_definitions(microdns PRIVATE HAVE_GETIFADDRS=1)
endif ()

check_include_file(ifaddrs.h HAVE_IFADDRS_H)
if (HAVE_IFADDRS_H)
    target_compile_definitions(microdns PRIVATE HAVE_IFADDRS_H=1)
endif ()

check_include_file(unistd.h HAVE_UNISTD_H)
if (HAVE_UNISTD_H)
    target_compile_definitions(microdns PRIVATE HAVE_UNISTD_H=1)
endif ()

