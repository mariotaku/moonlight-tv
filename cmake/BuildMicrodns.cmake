set(MICRODNS_DEP '')
set(MICRODNS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/libmicrodns")

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)


add_library(microdns
        ${MICRODNS_SOURCE_DIR}/src/mdns.c
        ${MICRODNS_SOURCE_DIR}/src/rr.c
        ${MICRODNS_SOURCE_DIR}/compat/compat.c
        ${MICRODNS_SOURCE_DIR}/compat/inet.c
        ${MICRODNS_SOURCE_DIR}/compat/poll.c
        )
target_include_directories(microdns SYSTEM PUBLIC ${MICRODNS_SOURCE_DIR}/include)
target_include_directories(microdns PRIVATE ${MICRODNS_SOURCE_DIR}/compat)

if (MINGW)
    target_link_libraries(microdns PRIVATE ws2_32 iphlpapi)
endif ()

check_c_source_compiles("
#ifdef _WIN32
#include <ws2tcpip.h>
#include <windows.h>
# if _WIN32_WINNT < 0x600
#  error Needs vista+
# endif
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
int main() {
inet_ntop(AF_INET, NULL, NULL, 0);
}
" HAVE_INET_NTOP)
if (HAVE_INET_NTOP)
    target_compile_definitions(microdns PRIVATE HAVE_INET_NTOP=1)
endif ()

check_c_source_compiles("
#include <stddef.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
# if _WIN32_WINNT < 0x600
#  error Needs vista+
# endif
# if defined(_MSC_VER)
#   error
# endif
#else
#include <poll.h>
#endif
int main() {
  poll(NULL, 0, 0);
}
" HAVE_POLL)
if (HAVE_POLL)
    target_compile_definitions(microdns PRIVATE HAVE_POLL=1)
endif ()

if (HAVE_POLL)
    set(CMAKE_EXTRA_INCLUDE_FILES "poll.h")
elseif (MSVC OR MINGW)
    set(CMAKE_EXTRA_INCLUDE_FILES "winsock2.h")
endif ()
check_type_size("struct pollfd" HAVE_STRUCT_POLLFD BUILTIN_TYPES_ONLY)
unset(CMAKE_EXTRA_INCLUDE_FILES)
if (HAVE_STRUCT_POLLFD)
    target_compile_definitions(microdns PRIVATE HAVE_STRUCT_POLLFD=1)
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

set_target_properties(microdns PROPERTIES SOVERSION 1)

install(TARGETS microdns LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})