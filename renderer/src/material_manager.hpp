#pragma once

#include <common/handles.hpp>
#include <common/id_generator.hpp>
#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

class DescriptorAllocator;
class ImageManager;
class SamplerManager;
class TextureManager;

class MaterialManager {
public:
  MaterialManager(VulkanContext& context, Descriptors& descriptors, TextureManager* texture_manager, ImageManager* image_manager, SamplerManager* sampler_manager);
  
  void load_material(Handle<Material> handle, Material& material);
  void unload_material(Handle<Material> handle);

  const VulkanMaterial& get_material(Handle<Material> handle) {return materials_.at(handle);}
  Handle<vk::DescriptorSetLayout> get_default_layout_handle() {return default_layout_;} 
private:
  VulkanContext* context_;
  Descriptors* descriptors_;
  ImageManager* image_manager_;
  SamplerManager* sampler_manager_;
  TextureManager* texture_manager_;

  std::unordered_map<Handle<Material>, VulkanMaterial> materials_;
  std::unordered_map<Handle<Material>, vk::raii::Buffer> material_buffers_;
  std::unordered_map<Handle<Material>, vk::raii::DeviceMemory> material_buffer_memory_;

  Handle<vk::DescriptorSetLayout> default_layout_;
};