add_executable(discard discard.cpp)
target_link_libraries(discard lynx)

add_executable(daytime daytime.cpp)
target_link_libraries(daytime lynx)

add_executable(time time.cpp)
target_link_libraries(time lynx)

add_executable(echo echo.cpp)
target_link_libraries(echo lynx)

add_executable(chargen chargen.cpp)
target_link_libraries(chargen lynx)

add_executable(timeclient timeclient.cpp)
target_link_libraries(timeclient lynx)

add_executable(chargenclient chargenclient.cpp)
target_link_libraries(chargenclient lynx)

add_executable(allinone allinone.cpp)
target_link_libraries(allinone lynx)
