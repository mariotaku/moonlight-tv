get_target_property(MOONLIGHT_COMMON_C_SOURCE_DIR moonlight-common-c SOURCE_DIR)

add_library(moonlight-common-c-headers INTERFACE)
target_include_directories(moonlight-common-c-headers INTERFACE ${MOONLIGHT_COMMON_C_SOURCE_DIR}/src ${MOONLIGHT_COMMON_C_SOURCE_DIR}/reedsolomon)

add_subdirectory(audio)
add_subdirectory(decoder)