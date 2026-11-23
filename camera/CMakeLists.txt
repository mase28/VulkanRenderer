add_library(camera STATIC
  src/camera.cpp
)

target_include_directories(camera 
  PUBLIC include
  PRIVATE src
)

target_link_libraries(camera PUBLIC glm::glm assets)