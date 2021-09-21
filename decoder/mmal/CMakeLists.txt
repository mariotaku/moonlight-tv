add_library(moonlight-mmal SHARED
        mmal_platform.c
        mmal_presenter.c
        mmalvid.c
        )

target_include_directories(moonlight-mmal PRIVATE
        ${CMAKE_SOURCE_DIR}/app/stream/audio
        ${CMAKE_SOURCE_DIR}/app/stream/video
        ${CMAKE_SOURCE_DIR}/app
        )

target_link_libraries(moonlight-mmal PRIVATE gamestream-sps ${BROADCOM_MMAL_LIBRARIES})

target_include_directories(moonlight-mmal PRIVATE ${BROADCOM_INCLUDE_DIRS})
target_compile_definitions(moonlight-mmal PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

target_link_libraries(moonlight-mmal PRIVATE moonlight-common-c opus)
target_include_directories(moonlight-mmal SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(moonlight-mmal PRIVATE ${SDL2_LIBRARIES})


install(TARGETS moonlight-mmal LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})