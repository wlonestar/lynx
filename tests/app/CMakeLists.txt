add_executable(application_test
  application_test.cpp
  favicon.cpp
  )
target_link_libraries(application_test lynx)

# Copy config file
file(
  COPY conf
  DESTINATION ${CMAKE_BINARY_DIR}/bin
  )
