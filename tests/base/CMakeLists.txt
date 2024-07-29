file(GLOB BASE_SRC "*_unittest.cpp")

foreach(SRC ${BASE_SRC})
  GET_FILENAME_COMPONENT(EXEC_NAME ${SRC} NAME_WE)
  add_executable(${EXEC_NAME} ${SRC})
  target_link_libraries(${EXEC_NAME} lynx boost_unit_test_framework)
  add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
endforeach()

add_executable(thread_bench thread_bench.cpp)
target_link_libraries(thread_bench lynx)

add_executable(thread_pool_bench thread_pool_bench.cpp)
target_link_libraries(thread_pool_bench lynx)
