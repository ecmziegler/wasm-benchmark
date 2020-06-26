project(bullet_benchmark
  DESCRIPTION "bullet benchmark"
  LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(bullet_bench "${THIRD_PARTY_DIR}/bullet/Demos/Benchmarks/BenchmarkDemo.cpp" "${THIRD_PARTY_DIR}/bullet/Demos/Benchmarks/main.cpp")

add_subdirectory("${THIRD_PARTY_DIR}/bullet" bullet EXCLUDE_FROM_ALL)
target_include_directories(bullet_bench PRIVATE "${THIRD_PARTY_DIR}/bullet" "${PROJECT_BINARY_DIR}/bullet")
target_link_libraries(bullet_bench PRIVATE bullet)
