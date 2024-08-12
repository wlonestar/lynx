cmake_minimum_required(VERSION 3.25)

project(lynx VERSION 1.0.0)

include(cmake/utils.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Set compiler flags
set(CXX_FLAGS
  -g -gdwarf-4 -std=c++20 -march=native
  -Wall -Wextra -Werror -Wno-unused-parameter -Wold-style-cast -Wshadow
  -Wno-null-dereference -Wno-sign-conversion -Wno-unused-local-typedef
  -Wthread-safety
  )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Set build params
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find Ragel
find_program(RAGEL_EXE NAMES ragel)
if (NOT RAGEL_EXE)
  message(FATAL_ERROR "Ragel executable not found")
endif()

# Enable ctest
enable_testing()

##################################################
# Find packages
##################################################

# Find Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})
# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})
# Find nlohmann::json
find_package(nlohmann_json 3.11.3 REQUIRED)
include_directories(${nlohmann_json_INCLUDE_DIRS})
# Find yaml-cpp
find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(tests)
add_subdirectory(examples)

# Copy config file
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/bin/conf/config.yml
  DESTINATION ${CMAKE_BINARY_DIR}/bin/conf
  )

# Set library source files
file(GLOB_RECURSE LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp)
# Generate ragel file
ragelmaker(lynx/http/http_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/http)
# Set build library
add_library(lynx STATIC ${LIB_SRC})
target_link_libraries(lynx PUBLIC
  pq
  nlohmann_json::nlohmann_json
  yaml-cpp
  )

##################################################
# Install
##################################################

# Set header files searching paths
# Distinguish between build-time and install-time header file paths
target_include_directories(lynx PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

# Install library and export config file
install(
  TARGETS lynx EXPORT lynxConfig
  LIBRARY DESTINATION lib
  )

# Install header files
# install(DIRECTORY lynx/ DESTINATION include/lynx FILES_MATCHING PATTERN "*.h")
FILE(GLOB BASE_HEADERS lynx/base/*.h)
FILE(GLOB DB_HEADERS lynx/db/*.h)
FILE(GLOB HTTP_HEADERS lynx/http/*.h)
FILE(GLOB LOGGER_HEADERS lynx/logger/*.h)
set(NET_HEADERS
  lynx/net/buffer.h
  lynx/net/channel.h
  lynx/net/event_loop.h
  lynx/net/event_loop_thread.h
  lynx/net/event_loop_thread_pool.h
  lynx/net/inet_address.h
  lynx/net/tcp_connection.h
  lynx/net/tcp_server.h
  )
FILE(GLOB ORM_HEADERS lynx/orm/*.h)
set(TIMER_HEADERS lynx/timer/timer_id.h)
FILE(GLOB WEB_HEADERS lynx/web/*.h)

install(FILES ${BASE_HEADERS}   DESTINATION include/lynx/base)
install(FILES ${DB_HEADERS}     DESTINATION include/lynx/db)
install(FILES ${HTTP_HEADERS}   DESTINATION include/lynx/http)
install(FILES ${LOGGER_HEADERS} DESTINATION include/lynx/logger)
install(FILES ${NET_HEADERS}    DESTINATION include/lynx/net)
install(FILES ${ORM_HEADERS}    DESTINATION include/lynx/orm)
install(FILES ${TIMER_HEADERS}  DESTINATION include/lynx/timer)
install(FILES ${WEB_HEADERS}    DESTINATION include/lynx/web)

# Install exported config file
install(EXPORT lynxConfig DESTINATION share/lynx/cmake)

##################################################
# Custom target
##################################################

# Set clang-format format
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if (CLANG_FORMAT_EXE)
  file(GLOB ALL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/lynx/*/*.h"
    "${CMAKE_SOURCE_DIR}/lib/*/*.cpp"
    )
  add_custom_target(clang-format
    COMMAND ${CLANG_FORMAT_EXE} -i --style=file:${CMAKE_SOURCE_DIR}/.clang-format ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on source files"
    )
endif()
