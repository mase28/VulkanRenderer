#include "command_pool.hpp"

CommandPool::CommandPool(VulkanContext& context, uint32_t queue_index, vk::CommandPoolCreateFlags flags) : context_(&context) {
  vk::CommandPoolCreateInfo pool_info {
    .flags = flags,
    .queueFamilyIndex = queue_index
  };
  pool_ = vk::raii::CommandPool(context_->device, pool_info);
}

std::vector<vk::raii::CommandBuffer> CommandPool::allocate_buffers(uint32_t count, vk::CommandBufferLevel level) {
  vk::CommandBufferAllocateInfo alloc_info {
    .commandPool = pool_,
    .level = level,
    .commandBufferCount = count
  };
  auto command_buffers = vk::raii::CommandBuffers(context_->device, alloc_info);
  if (command_buffers.empty()) {
    throw std::runtime_error("Unable to create command buffers");
  }
  return command_buffers;
}

vk::raii::CommandBuffer CommandPool::allocate_single_buffer(vk::CommandBufferLevel level) {
  vk::CommandBufferAllocateInfo alloc_info {
    .commandPool = pool_,
    .level = level,
    .commandBufferCount = 1
  };
  return std::move(context_->device.allocateCommandBuffers(alloc_info).front());
}