include(ExternalProject)

set(EXT_FDKAAC_TOOLCHAIN_ARGS)
if (CMAKE_TOOLCHAIN_FILE)
    list(APPEND EXT_FDKAAC_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_FILE:string=${CMAKE_TOOLCHAIN_FILE}")
endif ()
if (CMAKE_TOOLCHAIN_ARGS)
    list(APPEND EXT_FDKAAC_TOOLCHAIN_ARGS "-DCMAKE_TOOLCHAIN_ARGS:string=${CMAKE_TOOLCHAIN_ARGS}")
endif ()

set(LIB_FILENAME "${CMAKE_SHARED_LIBRARY_PREFIX}fdk-aac${CMAKE_SHARED_LIBRARY_SUFFIX}")

ExternalProject_Add(ext_fdkaac
        URL https://github.com/mstorsjo/fdk-aac/archive/refs/tags/v2.0.3.tar.gz
        URL_HASH SHA256=e25671cd96b10bad896aa42ab91a695a9e573395262baed4e4a2ff178d6a3a78
        CMAKE_ARGS ${EXT_OPUS_TOOLCHAIN_ARGS}
        -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${LIB_FILENAME}
        )
ExternalProject_Get_Property(ext_fdkaac INSTALL_DIR)

add_library(ext_fdkaac_target SHARED IMPORTED)
set_target_properties(ext_fdkaac_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/${LIB_FILENAME})

add_dependencies(ext_fdkaac_target ext_fdkaac)

set(FDKAAC_INCLUDE_DIRS ${INSTALL_DIR}/include)
set(FDKAAC_LIBRARIES ext_fdkaac_target)
set(FDKAAC_FOUND TRUE)

if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif ()

install(DIRECTORY ${INSTALL_DIR}/lib/ DESTINATION ${CMAKE_INSTALL_LIBDIR})
