file(GLOB HTTP_SRC "*_unittest.cpp")

foreach(SRC ${HTTP_SRC})
  GET_FILENAME_COMPONENT(EXEC_NAME ${SRC} NAME_WE)
  add_executable(${EXEC_NAME} ${SRC})
  target_link_libraries(${EXEC_NAME} lynx boost_unit_test_framework)
  add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
endforeach()

add_executable(http_server_test http_server_test.cpp)
target_link_libraries(http_server_test lynx)

