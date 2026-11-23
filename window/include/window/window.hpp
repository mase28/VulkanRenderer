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

  bool should_close();

  void poll_events();
  
  std::vector<const char*> get_extensions() {return extensions_;}

  VulkanWindowProviders get_vulkan_providers();

  private:
  SDL_Window* window_;
  std::vector<const char*> extensions_;

  bool resized_ = false;
};

