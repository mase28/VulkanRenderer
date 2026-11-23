#pragma once

#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

namespace utils {
  enum class QueueFamily {
    GRAPHICS,
    TRANSFER
  };
  
  void transition_image_layout(vk::raii::CommandBuffer& cmd_buffer, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels);

  void create_buffer(const VulkanContext* context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory);

  void create_image(const VulkanContext* context, uint32_t width, uint32_t height, uint32_t mip_levels, vk::SampleCountFlagBits num_samples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& image_memory);

  vk::Format find_supported_format(VulkanContext* context, const std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
}