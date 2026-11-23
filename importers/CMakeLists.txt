add_library(importers STATIC
  src/gltf_loader.cpp
  src/stb_impl.cpp
  src/tangent_calc.cpp
)

target_include_directories(importers
  PUBLIC include/
  PRIVATE src/
)

target_link_libraries(importers PUBLIC assets common)
target_link_libraries(importers PRIVATE glm::glm HeaderOnly fastgltf::fastgltf scene resource_manager mikktspace)
