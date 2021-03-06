add_library(moonlight-pi SHARED 
    pi_platform.c
    pi_presenter.c
    pivid.c
)

target_include_directories(moonlight-pi PRIVATE 
    ${CMAKE_SOURCE_DIR}/app/stream/audio
    ${CMAKE_SOURCE_DIR}/app/stream/video
    ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(moonlight-pi PRIVATE gamestream-sps ilclient ${BROADCOM_MMAL_LIBRARIES})

target_include_directories(moonlight-pi PRIVATE ${BROADCOM_INCLUDE_DIRS})
target_link_libraries(moonlight-pi PRIVATE ${BROADCOM_OMX_LIBRARIES})
target_compile_definitions(moonlight-pi PUBLIC ${BROADCOM_OMX_DEFINITIONS})
target_compile_definitions(moonlight-pi PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

target_link_libraries(moonlight-pi PRIVATE moonlight-common-c opus)
target_include_directories(moonlight-pi SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(moonlight-pi PRIVATE ${SDL2_LIBRARIES})


install(TARGETS moonlight-pi LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})