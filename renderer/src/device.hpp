#pragma once

#include <common/providers.hpp>
#include <renderer_types.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <cstdint>

class Device {
public:
  // add vulkan_window_providers
  Device(const std::vector<const char*>& extension_names, VulkanWindowProviders* providers, bool enable_validation_layers = false);

  void build_context(VulkanContext& context);

private:
  std::vector<const char*> extensions_;

  VulkanWindowProviders* window_providers;
  
  bool enable_validation_layers_ = false;
  std::vector<char const*> validation_layers_ {"VK_LAYER_KHRONOS_validation"};

  std::vector<const char*> required_extensions_ {vk::KHRSwapchainExtensionName, vk::KHRShaderDrawParametersExtensionName, vk::KHRBufferDeviceAddressExtensionName};

  // init functions
  bool create_instance(VulkanContext& context);
  bool setup_debug_messenger(VulkanContext& context);
  bool pick_physical_device(VulkanContext& context);
  bool create_logical_device(VulkanContext& context);

  // helpers
  uint32_t find_graphics_queue(vk::raii::PhysicalDevice& physical_device, VkSurfaceKHR surface);
  uint32_t find_transfer_queue(vk::raii::PhysicalDevice& physical_device);
};