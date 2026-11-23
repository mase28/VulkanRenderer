add_library(window STATIC
  ./src/window.cpp
)

# target_compile_definitions(window PRIVATE GLFW_INCLUDE_VULKAN)

target_include_directories(window 
  PUBLIC ./include
  PRIVATE ./src
)

target_link_libraries(window PUBLIC SDL3::SDL3 common)
target_link_libraries(window PRIVATE VulkanHeaders)