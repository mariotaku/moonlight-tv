add_unit_test(test_diag test_diag.c)
target_link_libraries(test_diag PRIVATE moonlight-test-e2e)