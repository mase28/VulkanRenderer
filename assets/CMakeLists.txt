add_library(assets INTERFACE)

target_include_directories(assets INTERFACE ./include)

target_link_libraries(assets INTERFACE glm::glm common)