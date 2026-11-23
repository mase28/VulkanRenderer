#include "window.hpp"
#include <common/providers.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_events.h>

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <memory>
#include <iostream>


Window::Window(const uint32_t width, const uint32_t height, const char* name) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Vulkan_LoadLibrary(nullptr);

  window_ = SDL_CreateWindow(name, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  SDL_SetWindowMouseGrab(window_, true);
  SDL_SetWindowRelativeMouseMode(window_, true);

  uint32_t extension_count;
  const char* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
  if (instance_extensions == nullptr) {
    throw std::runtime_error("Unable to get SDL window extensions");
  }
  extensions_.assign(instance_extensions, instance_extensions + extension_count);
}

Window::~Window() {
  SDL_DestroyWindow(window_);
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
}

VulkanWindowProviders Window::get_vulkan_providers() {
  SDL_Window* window = this->window_;
  auto surface_provider = [window](vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface) -> bool {
    VkSurfaceKHR _surface;
    if (!SDL_Vulkan_CreateSurface(window, *instance, NULL, &_surface)) {
      const char* error = SDL_GetError();
      std::cout << "Unable to create vulkan surface from SDL window. Error: " << error << std::endl;
      return false;
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
    return true;
  };
  auto window_size_provider = [window](int& width, int& height) -> bool {
    return SDL_GetWindowSize(window, &width, &height);
  };
  bool has_resized = resized_;
  auto size_changed = [has_resized]() -> bool {
    return has_resized;
  };
  auto wait_events = []() -> void {
    SDL_Event event;
    SDL_WaitEvent(&event);
  };

  VulkanWindowProviders providers = {
    .surface_provider = surface_provider,
    .window_size_provider = window_size_provider,
    .size_changed = size_changed,
    .wait_events = wait_events
  };

  return providers;
}