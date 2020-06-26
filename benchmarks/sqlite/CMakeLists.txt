project(sqlite_benchmark
  DESCRIPTION "sqlite benchmark"
  LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

include(../../CMakeLists.include)

if(PLATFORM STREQUAL "native")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl -lpthread")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s FILESYSTEM=1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s FILESYSTEM=1")
endif()

add_executable(sqlite_bench "${THIRD_PARTY_DIR}/sqlite/sqlite3.c" speedtest1.c)
target_include_directories(sqlite_bench PRIVATE "${THIRD_PARTY_DIR}/sqlite")
