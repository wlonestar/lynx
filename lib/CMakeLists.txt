file(GLOB LIB_SRC
  */*.cpp
  )

add_library(lynx SHARED 
  ${LIB_SRC}
  )
target_link_libraries(lynx PUBLIC 
  pthread
  ${PostgreSQL_LIBRARIES}
  )
