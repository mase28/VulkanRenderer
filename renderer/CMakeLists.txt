add_library(renderer
  src/renderer.cpp
  src/vulkan_renderer.cpp
  src/device.cpp
  src/swapchain.cpp
  src/descriptors.cpp
  src/pipeline_manager.cpp
  src/command_pool.cpp
  src/utils.cpp
  src/resource_loader.cpp
  src/instance_manager.cpp
  src/texture_manager.cpp
  src/material_manager.cpp
  src/mesh_manager.cpp
  src/image_manager.cpp
  src/sampler_manager.cpp
  src/imgui_impl.cpp
)

target_include_directories(renderer
  PUBLIC include/
  PRIVATE src/
)

target_link_libraries(renderer PUBLIC common assets)
target_link_libraries(renderer PRIVATE camera scene resource_manager VulkanHeaders glm::glm imgui)