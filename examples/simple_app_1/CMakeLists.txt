add_executable(simple-app-1 app.cpp favicon.cpp)
target_link_libraries(simple-app-1 lynx)

file(COPY conf DESTINATION ${CMAKE_BINARY_DIR}/bin)
