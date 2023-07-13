set(FIXTURES_PATH_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/")

function(add_unit_test NAME SOURCES)
    add_executable(${NAME} ${SOURCES})
    target_link_libraries(${NAME} PRIVATE unity moonlight-lib)
    target_compile_definitions(${NAME} PRIVATE FIXTURES_PATH_PREFIX="${FIXTURES_PATH_PREFIX}")
    add_test(${NAME} ${NAME})
endfunction()

add_subdirectory(core)
add_subdirectory(app)