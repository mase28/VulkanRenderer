#pragma once

#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

class CommandPool {
public:
  CommandPool(VulkanContext& context, uint32_t queue_index, vk::CommandPoolCreateFlags flags);

  std::vector<vk::raii::CommandBuffer> allocate_buffers(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

  vk::raii::CommandBuffer allocate_single_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

  const vk::CommandPool& handle() {return *pool_;};
private:
  VulkanContext* context_;
  vk::raii::CommandPool pool_ = nullptr;
};