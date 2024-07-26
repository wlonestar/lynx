cmake_minimum_required(VERSION 3.25)

project(application)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
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

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})
# Find nlohmann::json
find_package(nlohmann_json 3.11.3 REQUIRED)
# Find lynx
set(CMAKE_PREFIX_PATH "./lynx-1.0.0")
find_package(lynx CONFIG REQUIRED)
include_directories(${lynx_INCLUDE_DIRS})

# Add the executable
add_executable(app
  main.cpp
  favicon.cpp
)
target_link_libraries(app
  lynx
  pq
  nlohmann_json::nlohmann_json
  )
