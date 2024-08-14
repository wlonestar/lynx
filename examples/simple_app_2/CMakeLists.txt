add_executable(simple-app-2 app.cpp)
target_link_libraries(simple-app-2 lynx)

file(COPY conf DESTINATION ${CMAKE_BINARY_DIR}/bin)
