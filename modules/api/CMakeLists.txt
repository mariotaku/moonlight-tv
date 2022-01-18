add_library(moonlight-module-api INTERFACE)
target_include_directories(moonlight-module-api INTERFACE .)

get_target_property(MOONLIGHT_COMMON_C_SOURCE_DIR moonlight-common-c SOURCE_DIR)
target_include_directories(moonlight-module-api INTERFACE ${MOONLIGHT_COMMON_C_SOURCE_DIR}/src ${MOONLIGHT_COMMON_C_SOURCE_DIR}/reedsolomon)