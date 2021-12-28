set(INIH_DIR "${CMAKE_SOURCE_DIR}/third_party/inih")

add_library(inih STATIC ${INIH_DIR}/ini.c)
target_include_directories(inih PUBLIC ${INIH_DIR})