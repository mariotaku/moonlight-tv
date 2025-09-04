add_unit_test(test_pref_fps test_pref_fps.c)
target_link_libraries(test_pref_fps PRIVATE moonlight-test-e2e)

add_unit_test(test_pref_res test_pref_res.c)
target_link_libraries(test_pref_res PRIVATE moonlight-test-e2e)