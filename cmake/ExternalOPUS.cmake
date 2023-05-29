include(ExternalProject)

ExternalProject_Add(ext_opus
        URL https://downloads.xiph.org/releases/opus/opus-1.4.tar.gz
        URL_HASH SHA256=c9b32b4253be5ae63d1ff16eea06b94b5f0f2951b7a02aceef58e3a3ce49c51f
        CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE:string=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_TOOLCHAIN_ARGS:string=${CMAKE_TOOLCHAIN_ARGS}
        -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DOPUS_BUILD_SHARED_LIBRARY=ON
        -DOPUS_INSTALL_PKG_CONFIG_MODULE=OFF
        -DOPUS_INSTALL_CMAKE_CONFIG_MODULE=OFF
        )
ExternalProject_Get_Property(ext_opus INSTALL_DIR)

add_library(ext_opus_target SHARED IMPORTED)
set_target_properties(ext_opus_target PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libopus.so)
install(DIRECTORY ${INSTALL_DIR}/lib DESTINATION lib)

add_dependencies(ext_opus_target ext_opus)

set(OPUS_INCLUDE_DIRS ${INSTALL_DIR}/include/opus)
set(OPUS_LIBRARIES ext_opus_target)
