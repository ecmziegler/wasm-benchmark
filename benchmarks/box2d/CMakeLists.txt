project(box2d_benchmark
  DESCRIPTION "box2d benchmark"
  LANGUAGES CXX)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

add_executable(box2d_bench box2d_bench.cpp)

add_subdirectory("${THIRD_PARTY_DIR}/box2d" box2d EXCLUDE_FROM_ALL)
target_include_directories(box2d_bench PRIVATE "${THIRD_PARTY_DIR}/box2d")
if(PLATFORM STREQUAL "native")
  target_link_libraries(box2d_bench PRIVATE wasm_perf Box2D)
elseif(PLATFORM STREQUAL "wasm")
  target_link_libraries(box2d_bench PRIVATE Box2D)
  target_compile_options(box2d_bench PRIVATE --js-library "${JS_LIBRARY}")
  target_link_options(box2d_bench PRIVATE --js-library "${JS_LIBRARY}")
endif()