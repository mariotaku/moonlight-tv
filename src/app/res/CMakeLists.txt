target_sources(moonlight-lib PRIVATE res.c)


find_program(NPM_COMMAND npm)

if (NOT NPM_COMMAND)
    return()
endif ()

add_custom_target(moonlight-res VERBATIM
        COMMAND ${NPM_COMMAND} run gulp iconfont -- --output "${CMAKE_CURRENT_SOURCE_DIR}/gen" --input "${CMAKE_CURRENT_SOURCE_DIR}/iconfonts/*.ttf"
        COMMAND ${NPM_COMMAND} run gulp symlist -- --output "${CMAKE_SOURCE_DIR}/src/app/lvgl/font" --input "${CMAKE_CURRENT_SOURCE_DIR}/iconfonts/*.ttf"
        COMMAND ${NPM_COMMAND} run gulp binaries -- --output "${CMAKE_CURRENT_SOURCE_DIR}/gen" --input "${CMAKE_CURRENT_SOURCE_DIR}/img/*"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools)