#pragma once

#include <assets/mesh.hpp>
#include <common/handles.hpp>
#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

class ImageManager {
public:
  ImageManager(VulkanContext& context, CommandContext& command_context);

  void load_image(Handle<Image> handle, Image& image);
  void unload_image(Handle<Image> handle) {assert(handle.is_valid()); images_.erase(handle);}

  vk::Image get_image(Handle<Image> handle) {return *images_.at(handle).image;}
  vk::ImageView get_image_view(Handle<Image> handle) {return *images_.at(handle).image_view;}
private:
  VulkanContext* context_;
  CommandContext* command_context_;

  std::unordered_map<Handle<Image>, VulkanImage> images_;

  void copy_buffer_to_image(vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, uint32_t mip_levels);
  void generate_mipmaps(vk::raii::CommandBuffer& command_buffer, vk::raii::Image& image, vk::Format image_format, uint32_t width, uint32_t height, uint32_t mip_levels);

};