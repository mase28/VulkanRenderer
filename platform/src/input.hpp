#pragma once

#include <assets/math_types.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

class InputHandler {
public:
  //InputHandler();
  // only rotation for now
  glm::vec3 get_input();
  glm::vec2 get_mouse_delta();
private:

};