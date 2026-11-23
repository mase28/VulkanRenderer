add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_library(VulkanHeaders INTERFACE)
target_include_directories(VulkanHeaders INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_headers/include)
target_compile_definitions(VulkanHeaders INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
find_package(Vulkan REQUIRED)
target_link_libraries(VulkanHeaders INTERFACE Vulkan::Vulkan)

set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
set(GLM_FORCE_DEPTH_ZERO_TO_ONE ON CACHE BOOL "" FORCE)
add_subdirectory(glm)

add_library(HeaderOnly INTERFACE)
target_include_directories(HeaderOnly INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/includes)

set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "" FORCE)
add_subdirectory(fastgltf)

add_subdirectory(mikktspace)

add_subdirectory(imgui)

message(STATUS "All external libraries loaded")
