target_sources(moonlight PRIVATE
        lv_disp_drv_app.c
        lv_ext_utils.c
        ext/lv_child_group.c
        util/lv_app_utils.c
        font/font_empty.c
        theme/lv_theme_moonlight.c)
add_subdirectory(input)