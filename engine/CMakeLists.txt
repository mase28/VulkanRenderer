add_library(engine STATIC
  ./src/engine.cpp
  ./src/ui.cpp
)

target_include_directories(engine
  PUBLIC ./include
  PRIVATE ./src
)

target_link_libraries(engine PUBLIC platform renderer resource_manager scene camera common importers)
target_link_libraries(engine PRIVATE imgui)
