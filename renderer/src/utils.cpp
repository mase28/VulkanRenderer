#include "utils.hpp"

void utils::transition_image_layout(vk::raii::CommandBuffer& cmd_buffer, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels) {
  vk::ImageMemoryBarrier2 barrier {
    .srcStageMask = src_stage_mask,
    .srcAccessMask = src_access_mask,
    .dstStageMask = dst_stage_mask,
    .dstAccessMask = dst_access_mask,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {.aspectMask = aspect_flags, .baseMipLevel = 0, .levelCount = mip_levels, .baseArrayLayer = 0, .layerCount = 1}
  };
  vk::DependencyInfo dependency_info {
    .dependencyFlags = {},
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
  };
  cmd_buffer.pipelineBarrier2(dependency_info);
}

uint32_t find_memory_type(uint32_t type_filter, const vk::raii::PhysicalDevice& physical_device, vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties mem_properties = physical_device.getMemoryProperties();
  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags) == properties) {
      return i;
    }
  }
  throw std::runtime_error("Unable to find suitable memory type");
}

void utils::create_buffer(const VulkanContext* context, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory) {
  vk::BufferCreateInfo create_info {
    .size = size,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
  };
  buffer = vk::raii::Buffer{context->device, create_info};

  vk::MemoryRequirements mem_requirements = buffer.getMemoryRequirements();
  vk::MemoryAllocateFlagsInfo flags_info {
    .flags = vk::MemoryAllocateFlagBits::eDeviceAddress,
    .deviceMask = 0
  };
  vk::MemoryAllocateInfo mem_alloc_info {
    .pNext = &flags_info,
    .allocationSize = mem_requirements.size,
    .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, context->physical_device, properties)
  };
  buffer_memory = vk::raii::DeviceMemory{context->device, mem_alloc_info};

  buffer.bindMemory(*buffer_memory, 0);
}

void utils::create_image(const VulkanContext* context, uint32_t width, uint32_t height, uint32_t mip_levels, vk::SampleCountFlagBits num_samples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& image_memory) {
  vk::ImageCreateInfo create_info {
    .imageType = vk::ImageType::e2D,
    .format = format,
    .extent = {width, height, 1},
    .mipLevels = mip_levels,
    .arrayLayers = 1,
    .samples = num_samples,
    .tiling = tiling,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
  };

  image = vk::raii::Image(context->device, create_info);

  vk::MemoryRequirements mem_requirements = image.getMemoryRequirements();
  vk::MemoryAllocateInfo alloc_info {
    .allocationSize = mem_requirements.size,
    .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, context->physical_device, properties)
  };
  image_memory = vk::raii::DeviceMemory(context->device, alloc_info);
  image.bindMemory(image_memory, 0);
}

vk::Format utils::find_supported_format(VulkanContext* context, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props = context->physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
