#pragma once

//#include <common/handles.hpp>
#include <common/providers.hpp>
#include <assets/math_types.hpp>

#include <cstdint>
#include <vector>
#include <memory>

class Window;
class InputHandler;

class Platform {
public:
  Platform();
  ~Platform();
  void poll();

  // window functions
  bool should_close() {return quit_requested;}
  std::vector<const char*> get_window_extensions();
  VulkanWindowProviders get_window_providers();

  //input functionality
  glm::vec2 get_mouse_delta();
  glm::vec3 get_input();

  // for imgui
  void new_frame();
private:
  std::unique_ptr<Window> main_window_ = nullptr;
  std::unique_ptr<InputHandler> input_handler_ = nullptr;

  bool quit_requested = false;


  glm::vec2 mouse_delta_;
  glm::vec3 input_;
};
