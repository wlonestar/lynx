file(GLOB NET_SRC "*_unittest.cpp")

foreach(SRC ${NET_SRC})
  GET_FILENAME_COMPONENT(EXEC_NAME ${SRC} NAME_WE)
  add_executable(${EXEC_NAME} ${SRC})
  target_link_libraries(${EXEC_NAME} lynx boost_unit_test_framework)
  add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
endforeach()

add_executable(channel_bench channel_bench.cpp)
target_link_libraries(channel_bench lynx)

add_executable(event_loop_test event_loop_test_fail.cpp)
target_link_libraries(event_loop_test lynx)

add_executable(event_loop_thread_pool_bench event_loop_thread_pool_bench.cpp)
target_link_libraries(event_loop_thread_pool_bench lynx)

add_executable(echo_server_test echo_server_test.cpp)
target_link_libraries(echo_server_test lynx)

add_executable(echo_client_test echo_client_test.cpp)
target_link_libraries(echo_client_test lynx)
