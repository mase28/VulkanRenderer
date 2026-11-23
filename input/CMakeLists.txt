add_library(input STATIC
  src/input.cpp
)

target_include_directories(input 
  PUBLIC include
  PRIVATE src
)

target_link_libraries(input PUBLIC assets glm::glm)
target_link_libraries(input PRIVATE SDL3::SDL3)