add_library(resource_manager STATIC
  src/resource_manager.cpp
)

target_include_directories(resource_manager
  PUBLIC include/
  PRIVATE src/
)

target_link_libraries(resource_manager PUBLIC assets common)
target_link_libraries(resource_manager PRIVATE glm::glm HeaderOnly fastgltf::fastgltf)