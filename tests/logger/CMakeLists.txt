file(GLOB LOGGER_SRC "*_unittest.cpp")

foreach(SRC ${LOGGER_SRC})
  GET_FILENAME_COMPONENT(EXEC_NAME ${SRC} NAME_WE)
  add_executable(${EXEC_NAME} ${SRC})
  target_link_libraries(${EXEC_NAME} lynx boost_unit_test_framework)
  add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
endforeach()

add_executable(async_logging_test async_logging_test.cpp)
target_link_libraries(async_logging_test lynx)

add_executable(log_stream_bench log_stream_bench.cpp)
target_link_libraries(log_stream_bench lynx)
