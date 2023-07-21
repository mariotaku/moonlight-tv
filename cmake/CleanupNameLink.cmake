file(GLOB FOUND_LIBS "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/**/*.so*")

function(get_soname PATH)
    set(SONAME PARENT_SCOPE)
    execute_process(COMMAND objdump -p ${PATH} OUTPUT_VARIABLE OBJDUMP_OUTPUT COMMAND_ERROR_IS_FATAL ANY)
    string(REGEX MATCH [[SONAME +([^ ]+)]] SONAME_MATCHES "${OBJDUMP_OUTPUT}")
    if (SONAME_MATCHES)
        string(STRIP "${CMAKE_MATCH_1}" SONAME)
        set(SONAME ${SONAME} PARENT_SCOPE)
    endif ()
endfunction()

foreach (FOUND_LIB ${FOUND_LIBS})
    cmake_path(GET FOUND_LIB FILENAME LIB_BASENAME)
    cmake_path(GET FOUND_LIB PARENT_PATH LIB_DIRNAME)
    if (IS_SYMLINK ${FOUND_LIB})
        message("Delete symlink ${LIB_BASENAME}")
        file(REMOVE ${FOUND_LIB})
    else ()
        get_soname(${FOUND_LIB})
        if (NOT SONAME STREQUAL LIB_BASENAME)
            message("Rename ${LIB_BASENAME} => ${SONAME}")
            file(RENAME ${FOUND_LIB} ${LIB_DIRNAME}/${SONAME})
        endif ()
    endif ()
endforeach ()