add_library(moonlight-test-e2e STATIC base_impl.c e2e_lv_cursor_img.c)
target_include_directories(moonlight-test-e2e PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(moonlight-test-e2e PRIVATE unity moonlight-lib)