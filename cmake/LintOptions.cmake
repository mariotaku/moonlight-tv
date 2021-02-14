cmake_minimum_required (VERSION 3.6)

function(target_compile_lint TARGET)
  if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # using Clang
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TARGET} PRIVATE -Werror=return-type)
  endif()
endfunction()