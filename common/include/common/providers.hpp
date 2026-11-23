#pragma once

#include <functional>

namespace vk::raii {
  class Instance;
  class SurfaceKHR;
}

struct VulkanWindowProviders{
  std::function<bool(vk::raii::Instance&, vk::raii::SurfaceKHR&)> surface_provider;
  std::function<bool(int&, int&)> window_size_provider;
  std::function<bool()> size_changed;
  std::function<void()> wait_events;
};