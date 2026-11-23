#pragma once

#include <assets/math_types.hpp>

class InputHandler {
public:
  //InputHandler();
  // only rotation for now
  glm::vec3 get_input();
  glm::vec2 get_mouse_location();
private:

};