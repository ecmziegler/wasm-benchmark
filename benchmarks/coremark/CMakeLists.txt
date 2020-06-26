project(coremark_benchmark
  DESCRIPTION "coremark benchmark"
  LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(coremark_bench "${THIRD_PARTY_DIR}/coremark/core_main.c")

add_custom_target(coremark
  COMMAND ${MAKE_COMMAND} WORKING_DIRECTORY "${THIRD_PARTY_DIR}/coremark")
add_custom_command(TARGET coremark POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy coremark.a "${PROJECT_BINARY_DIR}/libcoremark.a"
  COMMAND ${MAKE_COMMAND} clean
  WORKING_DIRECTORY "${THIRD_PARTY_DIR}/coremark")
add_dependencies(coremark_bench coremark)
target_include_directories(coremark_bench PRIVATE "${THIRD_PARTY_DIR}/coremark")
target_link_libraries(coremark_bench PRIVATE "${PROJECT_BINARY_DIR}/libcoremark.a")
