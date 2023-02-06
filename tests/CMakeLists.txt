function(add_unit_test NAME SOURCES)
    add_executable(${NAME} ${SOURCES})
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/app)
    target_link_libraries(${NAME} PRIVATE unity)
    add_test(${NAME} ${NAME})
endfunction()

add_subdirectory(core)
add_subdirectory(app)