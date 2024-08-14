add_executable(simple-app-3 app.cpp)
target_link_libraries(simple-app-3 lynx)

file(COPY conf DESTINATION ${CMAKE_BINARY_DIR}/bin)
