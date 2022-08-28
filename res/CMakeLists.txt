set(RES
        moonlight.png
        defcover.png
        fav_indicator.png
        ${CMAKE_SOURCE_DIR}/app/lvgl/font/material_icons_regular.ttf)

add_custom_target(moonlight-res
        COMMAND ${NPM_COMMAND} run gen-res-headers -- -i ${RES} -o ${CMAKE_SOURCE_DIR}/app/res -d ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES ${RES})
add_dependencies(moonlight moonlight-res)