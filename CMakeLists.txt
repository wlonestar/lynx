cmake_minimum_required(VERSION 3.25)

project(lynx VERSION 1.0.0)

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
  -Wno-null-dereference -Wno-sign-conversion -Wno-unused-local-typedef -Wthread-safety
  -fsanitize=address,undefined -fno-omit-frame-pointer
  )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Set build params
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set clang-tidy check
# find_program(CLANG_TIDY_EXE NAMES clang-tidy)
# if (CLANG_TIDY_EXE)
#   set(CLANG_TIDY_OPTIONS "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
#   set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${CLANG_TIDY_OPTIONS}")
# endif()

# Set clang-format format
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if (CLANG_FORMAT_EXE)
  file(GLOB_RECURSE ALL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/lynx/*.h"
    "${CMAKE_SOURCE_DIR}/lib/*.cpp"
    )

  add_custom_target(clang-format
    COMMAND ${CLANG_FORMAT_EXE} -i --style=file:${CMAKE_SOURCE_DIR}/.clang-format ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on source files"
    )
endif()

# Find Ragel
find_program(RAGEL_EXE NAMES ragel)
if (NOT RAGEL_EXE)
  message(FATAL_ERROR "Ragel executable not found")
endif()

# Enable ctest
enable_testing()

# Find Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

# Find nlohmann::json
find_package(nlohmann_json 3.11.3 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(tests)

# Generate ragel file
function(ragelmaker src_rl outputlist outputdir)
  #Create a custom build step that will call ragel on the provided src_rl file.
  #The output .cpp file will be appended to the variable name passed in outputlist.

  get_filename_component(src_file ${src_rl} NAME_WE)
  set(rl_out ${outputdir}/${src_file}.cpp)
  set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

  add_custom_command(
    OUTPUT ${rl_out}
    COMMAND cd ${outputdir}
    COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -L -l -C -F1 -G2
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
    )
  set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
endfunction(ragelmaker)

# Set library source files
file(GLOB_RECURSE LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp)

ragelmaker(lynx/http/http_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/lib/http)

add_library(lynx STATIC ${LIB_SRC})
target_link_libraries(lynx PUBLIC pthread pq nlohmann_json::nlohmann_json)

# 
# Install
# 

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
install(DIRECTORY lynx/ 
  DESTINATION include/lynx 
  FILES_MATCHING PATTERN "*.h"
  )

# Install exported config file
install(EXPORT lynxConfig DESTINATION share/lynx/cmake)
