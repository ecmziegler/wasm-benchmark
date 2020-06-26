project(box2d_benchmark
  DESCRIPTION "box2d benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(box2d_bench box2d_bench.cpp)

add_subdirectory("${THIRD_PARTY_DIR}/box2d" box2d EXCLUDE_FROM_ALL)
target_include_directories(box2d_bench PRIVATE "${THIRD_PARTY_DIR}/box2d")
target_link_libraries(box2d_bench PRIVATE Box2D)
