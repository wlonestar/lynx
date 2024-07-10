cmake_minimum_required(VERSION 3.25)

project(lynx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_COMPILER "/usr/bin/clang-16")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-16")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(CXX_FLAGS
  -g -gdwarf-4 -std=c++20 -march=native
  -Wall -Wextra -Werror -Wno-unused-parameter -Wold-style-cast -Wshadow
  -Wno-null-dereference -Wno-sign-conversion -Wno-unused-local-typedef -Wthread-safety
  -fsanitize=address,undefined -fno-omit-frame-pointer
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

enable_testing()

find_package(Boost REQUIRED)
find_package(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)
