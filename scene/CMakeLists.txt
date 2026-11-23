add_library(scene STATIC
  src/scene.cpp
)

target_include_directories(scene
  PUBLIC include/
  PRIVATE src/
)

target_link_libraries(scene PUBLIC assets common)
target_link_libraries(scene PRIVATE glm)