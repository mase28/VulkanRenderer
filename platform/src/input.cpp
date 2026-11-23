#include "input.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

glm::vec3 InputHandler::get_input() {
  const bool* key_states = SDL_GetKeyboardState(NULL);
  glm::vec3 input = {0,0,0};
  
  if (key_states[SDL_SCANCODE_A]) {
    input[0] -= 1;
  }
  if (key_states[SDL_SCANCODE_D]) {
    input[0] += 1;
  }

  if (key_states[SDL_SCANCODE_W]) {
    input[2] += 1;
  }
  if (key_states[SDL_SCANCODE_S]) {
    input[2] -= 1;
  }

  if (key_states[SDL_SCANCODE_SPACE]) {
    input[1] += 1;
  }
  if (key_states[SDL_SCANCODE_LCTRL]) {
    input[1] -= 1;
  }

  return input;
}

glm::vec2 InputHandler::get_mouse_delta() {
  float x, y;
  SDL_GetRelativeMouseState(&x, &y);
  return glm::vec2 {x, y};
}