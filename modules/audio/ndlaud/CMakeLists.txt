cmake_minimum_required (VERSION 3.6)

add_library(moonlight-ndlaud SHARED 
    module.c
    ndl_audio.c
)
target_include_directories(moonlight-ndlaud PRIVATE ${CMAKE_SOURCE_DIR}/app)
target_link_libraries(moonlight-ndlaud PRIVATE moonlight-common-c-headers opus NDL_directmedia)
target_compile_definitions(moonlight-ndlaud PRIVATE WEBOS_TARGET_VERSION_MAJOR=4 "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS moonlight-ndlaud LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})