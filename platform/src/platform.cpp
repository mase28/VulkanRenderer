#include "input.hpp"
#include "window.hpp"
#include <platform/platform.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

Platform::Platform() {
  main_window_ = std::make_unique<Window>(1600, 800, "Main Window");
  input_handler_ = std::make_unique<InputHandler>();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplSDL3_InitForVulkan(main_window_->get_window_handle());
}

Platform::~Platform() {
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void Platform::poll() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    switch (event.type) {
    case SDL_EventType::SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      quit_requested = true;
      break;
    case SDL_EventType::SDL_EVENT_MOUSE_MOTION:
      mouse_delta_ = input_handler_->get_mouse_delta();
      break;
    case SDL_EventType::SDL_EVENT_KEY_DOWN:
      input_ = input_handler_->get_input();
      break;
    }
  }
}

std::vector<const char *> Platform::get_window_extensions() {
  return main_window_->get_vulkan_extensions();
}

VulkanWindowProviders Platform::get_window_providers() {
  return main_window_->get_vulkan_providers();
}

glm::vec2 Platform::get_mouse_delta() {
  glm::vec2 mouse_delta = mouse_delta_;
  mouse_delta_ = {0, 0};
  return mouse_delta;
}

glm::vec3 Platform::get_input() {
  glm::vec3 input = input_;
  input_ = {0, 0, 0};
  return input;
}

void Platform::new_frame() { ImGui_ImplSDL3_NewFrame(); };
