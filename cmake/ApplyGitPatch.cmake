foreach (_required_var GIT_EXECUTABLE SOURCE_DIR PATCH_FILE)
    if (NOT DEFINED ${_required_var})
        message(FATAL_ERROR "${_required_var} is required")
    endif ()
endforeach ()

# ExternalProject's git update step runs again when CPack builds the preinstall
# target. Treat an already-applied patch as success so repeated builds remain
# idempotent, while still failing if the source matches neither side.
execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero --reverse --check "${PATCH_FILE}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE _reverse_check_result
        OUTPUT_QUIET
        ERROR_QUIET)
if (_reverse_check_result EQUAL 0)
    return()
endif ()

execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero --check "${PATCH_FILE}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE _check_result)
if (NOT _check_result EQUAL 0)
    message(FATAL_ERROR "Patch cannot be applied cleanly: ${PATCH_FILE}")
endif ()

execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply --unidiff-zero "${PATCH_FILE}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY)
