add_library(common STATIC
  src/id_generator.cpp
)

target_include_directories(common 
  PUBLIC ./include
  PRIVATE ./src  
)

#target_link_libraries(common PRIVATE VulkanHeaders)