#include <renderer/renderer.hpp>
#include "vulkan_renderer.hpp"

#include <memory>

std::unique_ptr<Renderer> create_renderer(const VulkanRendererConfig &config) {
  return std::make_unique<VulkanRenderer>(config);
}