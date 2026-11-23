#include "image_manager.hpp"
#include "command_pool.hpp"
#include "utils.hpp"

ImageManager::ImageManager(VulkanContext& context, CommandContext& command_context) : context_(&context), command_context_(&command_context) {

}

void ImageManager::load_image(Handle<Image> handle, Image& image) {
  VulkanImage vk_image;

  vk::DeviceSize image_size = sizeof(image.pixels[0]) * image.pixels.size();
  uint32_t mip_levels = std::floor(std::log2(std::max(image.width, image.height))) + 1;

  vk::raii::Buffer staging_buffer = nullptr;
  vk::raii::DeviceMemory staging_buffer_memory = nullptr;
  utils::create_buffer(context_, image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, staging_buffer, staging_buffer_memory);

  void* data_staging = staging_buffer_memory.mapMemory(0, image_size);
  memcpy(data_staging, image.pixels.data(), image_size);
  staging_buffer_memory.unmapMemory();

  utils::create_image(context_, image.width, image.height, mip_levels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk_image.image, vk_image.memory);
  copy_buffer_to_image(staging_buffer, vk_image.image, image.width, image.height, mip_levels);

  vk::ImageViewCreateInfo view_info {
    .image = vk_image.image,
    .viewType = vk::ImageViewType::e2D,
    .format = vk::Format::eR8G8B8A8Srgb,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1}
  };
  vk_image.image_view = vk::raii::ImageView(context_->device, view_info);

  images_.try_emplace(handle, std::move(vk_image));
}

void ImageManager::copy_buffer_to_image(vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, uint32_t mip_levels) {
  // acquire single use command buffer from transfer queue
  vk::raii::CommandBuffer transfer_command = command_context_->transfer_pool->allocate_single_buffer();
  
  vk::CommandBufferBeginInfo begin_info {
    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
  };
  transfer_command.begin(begin_info);

  // first transfer image layout to transfer dst optimal for copying the buffer into the image on device 
  // only one mip level bc mip levels haven't been generated yet
  vk::ImageMemoryBarrier2 transition_to_transfer_dst {
    .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
    .srcAccessMask = vk::AccessFlagBits2::eNone,
    .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
    .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
    .oldLayout = vk::ImageLayout::eUndefined,
    .newLayout = vk::ImageLayout::eTransferDstOptimal,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1}
  };
  vk::DependencyInfo transition_to_transfer_dependency_info {
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &transition_to_transfer_dst
  };

  transfer_command.pipelineBarrier2(transition_to_transfer_dependency_info);

  // use whole image
  vk::BufferImageCopy region {
    .bufferOffset = 0,
    .bufferRowLength = 0,
    .bufferImageHeight = 0,
    .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
    .imageOffset = {0, 0, 0},
    .imageExtent = {width, height, 1}
  };

  // copy buffer to image with layout equal to transfer dst optimal
  transfer_command.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

  // now release image to graphics queue and transition layout to transfer src optimal for mip map generation
  // only one mip level since mip levels haven't been generated yet
  vk::ImageMemoryBarrier2 release_barrier {
    .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eNone,
    .dstAccessMask = vk::AccessFlagBits2::eNone,
    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    .newLayout = vk::ImageLayout::eTransferDstOptimal,
    .srcQueueFamilyIndex = context_->queue_indices.transfer,
    .dstQueueFamilyIndex = context_->queue_indices.graphics,
    .image = image,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1}
  };
  vk::DependencyInfo release_dependency_info {
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &release_barrier
  };
  transfer_command.pipelineBarrier2(release_dependency_info);

  transfer_command.end();

  vk::SubmitInfo transfer_submit {
    .commandBufferCount = 1,
    .pCommandBuffers = &*transfer_command
  };

  // Acquire for graphics queue, generate mip maps, mip map generation will transfer to shader read
  vk::raii::CommandBuffer graphics_command = command_context_->graphics_pool->allocate_single_buffer();
  graphics_command.begin(begin_info);

  vk::ImageMemoryBarrier2 acquire_barrier {
    .srcStageMask = vk::PipelineStageFlagBits2::eNone,
    .srcAccessMask = vk::AccessFlagBits2::eNone,
    .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
    .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    .newLayout = vk::ImageLayout::eTransferDstOptimal,
    .srcQueueFamilyIndex = context_->queue_indices.transfer,
    .dstQueueFamilyIndex = context_->queue_indices.graphics,
    .image = image,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1}
  };
  vk::DependencyInfo graphics_dependency_info {
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &acquire_barrier
  };
  graphics_command.pipelineBarrier2(graphics_dependency_info);

  generate_mipmaps(graphics_command, image, vk::Format::eR8G8B8A8Srgb, width, height, mip_levels);
  
  graphics_command.end();

  vk::SubmitInfo graphics_submit {
    .commandBufferCount = 1,
    .pCommandBuffers = &*graphics_command
  };

  // Now we do the submission and use fence for graphics queue to wait on transfer
  auto fence = context_->device.createFence({});
  
  context_->queues.transfer.submit(transfer_submit, fence);
  auto transfer_fence_result = context_->device.waitForFences(*fence, true, UINT64_MAX);
  if (transfer_fence_result != vk::Result::eSuccess) {
    throw std::runtime_error("Unable to wait for transfer fence");
  }
  context_->device.resetFences(*fence);
  
  context_->queues.graphics.submit(graphics_submit, fence);
  auto graphics_fence_result = context_->device.waitForFences(*fence, true, UINT64_MAX);
  if (graphics_fence_result != vk::Result::eSuccess) {
    throw std::runtime_error("Unable to wait for graphics fence");
  }

  //std::cout << "Texture image has been loaded into GPU memory\n";
}

void ImageManager::generate_mipmaps(vk::raii::CommandBuffer& command_buffer, vk::raii::Image& image, vk::Format image_format, uint32_t width, uint32_t height, uint32_t mip_levels) {
  vk::FormatProperties format_properties = context_->physical_device.getFormatProperties(image_format);
  if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    throw std::runtime_error("Texture image does not support linear blitting");
  }

  vk::ImageMemoryBarrier barrier {
    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
    .dstAccessMask = vk::AccessFlagBits::eTransferRead,
    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    .newLayout = vk::ImageLayout::eTransferSrcOptimal,
    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
    .image = image,
    .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
  };

  int32_t mip_width = width;
  int32_t mip_height = height;
  for (uint32_t i = 1; i < mip_levels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

    vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dst_offsets;
    offsets[0] = vk::Offset3D {0, 0, 0};
    offsets[1] = vk::Offset3D {mip_width, mip_height, 1};
    dst_offsets[0] = vk::Offset3D {0, 0, 0};
    dst_offsets[1] = vk::Offset3D {
      mip_width > 1 ? mip_width / 2 : 1,
      mip_height > 1 ? mip_height / 2 : 1,
      1
    };
    vk::ImageBlit blit = {
      .srcSubresource = {vk::ImageAspectFlagBits::eColor, i-1, 0, 1},
      .srcOffsets = offsets,
      .dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1},
      .dstOffsets = dst_offsets
    };
    command_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    if (mip_width > 1) mip_width /= 2;
    if (mip_height > 1) mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
  command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
}