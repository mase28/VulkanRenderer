add_library(imgui STATIC
  imgui.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
  backends/imgui_impl_sdl3.cpp
  backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui PUBLIC ./)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
target_link_libraries(imgui PRIVATE VulkanHeaders)
