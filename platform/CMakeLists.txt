add_library(platform STATIC
  src/platform.cpp
  src/window.cpp
  src/input.cpp
)

target_include_directories(platform
  PUBLIC include
  PRIVATE src
)

target_link_libraries(platform PUBLIC assets common)
target_link_libraries(platform PRIVATE glm::glm VulkanHeaders SDL3::SDL3 imgui)
