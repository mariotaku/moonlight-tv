function(add_unit_test NAME SOURCES)
    add_executable(${NAME} ${SOURCES})
    target_link_libraries(${NAME} PRIVATE unity moonlight-lib)
    add_test(${NAME} ${NAME})
endfunction()

add_subdirectory(core)
add_subdirectory(app)