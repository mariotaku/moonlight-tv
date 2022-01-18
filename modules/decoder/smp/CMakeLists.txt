enable_language(CXX OPTIONAL)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)

set(CMAKE_CXX_STANDARD 11)
add_library(moonlight-smp-webos3 SHARED
    src/DecoderPlatform.cpp
    src/AVStreamPlayer.cpp
    src/base64.cpp
)

target_include_directories(moonlight-smp-webos3 PRIVATE ${GLIB_INCLUDE_DIRS} ${PBNJSON_CPP_INCLUDE_DIRS})
target_include_directories(moonlight-smp-webos3 PRIVATE src)
target_link_libraries(moonlight-smp-webos3 PRIVATE moonlight-module-api playerAPIs Acb ${PBNJSON_CPP_LIBRARIES})
target_compile_definitions(moonlight-smp-webos3 PRIVATE USE_ACB PLUGIN_SYMBOL_SUFFIX=smp_webos3 SMP_DECODER_NS=MoonlightStarfish_webOS3)
target_compile_definitions(moonlight-smp-webos3 PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
# webOS 3 and before don't have GLIBCXX_3.4.21
target_compile_definitions(moonlight-smp-webos3 PRIVATE _GLIBCXX_USE_CXX11_ABI=0)

install(TARGETS moonlight-smp-webos3 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

set(CMAKE_CXX_STANDARD 11)
add_library(moonlight-smp-webos4 SHARED
    src/DecoderPlatform.cpp
    src/AVStreamPlayer.cpp
    src/base64.cpp
)
target_include_directories(moonlight-smp-webos4 PRIVATE ${GLIB_INCLUDE_DIRS} ${PBNJSON_CPP_INCLUDE_DIRS})
target_include_directories(moonlight-smp-webos4 PRIVATE src)
target_link_libraries(moonlight-smp-webos4 PRIVATE moonlight-module-api playerAPIs Acb ${PBNJSON_CPP_LIBRARIES})
target_compile_definitions(moonlight-smp-webos4 PRIVATE USE_ACB PLUGIN_SYMBOL_SUFFIX=smp_webos4 SMP_DECODER_NS=MoonlightStarfish_webOS4)
target_compile_definitions(moonlight-smp-webos4 PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-smp-webos4 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

unset(CMAKE_CXX_STANDARD)

add_library(moonlight-smp SHARED
    src/DecoderPlatform.cpp
    src/AVStreamPlayer.cpp
    src/base64.cpp
)
target_include_directories(moonlight-smp PRIVATE ${SDL2_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${PBNJSON_CPP_INCLUDE_DIRS})
target_include_directories(moonlight-smp PRIVATE src)
target_link_libraries(moonlight-smp PRIVATE moonlight-module-api playerAPIs ${SDL2_LIBRARIES} ${PBNJSON_CPP_LIBRARIES})
target_compile_definitions(moonlight-smp PRIVATE USE_SDL_WEBOS PLUGIN_SYMBOL_SUFFIX=smp SMP_DECODER_NS=MoonlightStarfish)
target_compile_definitions(moonlight-smp PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-smp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})