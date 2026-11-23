#pragma once

#include <common/providers.hpp>

#include <cstdint>
#include <vector>
#include <array>

#include <SDL3/SDL.h>

class Window {
  public:
  Window(const uint32_t width, const uint32_t height, const char* name);
  ~Window();

  std::vector<const char*> get_vulkan_extensions() {return extensions_;}

  VulkanWindowProviders get_vulkan_providers();

  SDL_Window* get_window_handle() {return window_;}

  private:
  SDL_Window* window_;
  std::vector<const char*> extensions_;

  bool resized_ = false;
};
