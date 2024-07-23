add_executable(web_server_test
  web_server_test.cpp
  favicon.cpp
  )
target_link_libraries(web_server_test lynx)
