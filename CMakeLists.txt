cmake_minimum_required(VERSION 4.2.1)

set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CXX "/usr/lib64/ccache/clang++")

project("VulkanRenderer" VERSION 0.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external)

add_subdirectory(common)

add_subdirectory(assets)

add_subdirectory(platform)

add_subdirectory(resource_manager)

add_subdirectory(scene)

add_subdirectory(importers)

add_subdirectory(camera)

add_subdirectory(renderer)

add_subdirectory(engine)
