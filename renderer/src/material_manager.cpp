#include "material_manager.hpp"
#include "descriptors.hpp"
#include "image_manager.hpp"
#include "sampler_manager.hpp"
#include "texture_manager.hpp"
#include "utils.hpp"

#include <iostream>

MaterialManager::MaterialManager(VulkanContext& context, Descriptors& descriptors, TextureManager* texture_manager, ImageManager* image_manager, SamplerManager* sampler_manager) : context_(&context), descriptors_(&descriptors), texture_manager_(texture_manager), image_manager_(image_manager), sampler_manager_(sampler_manager){
  vk::DescriptorSetLayoutBinding albedo_binding {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment,
    .pImmutableSamplers = nullptr
  };
  vk::DescriptorSetLayoutBinding normal_binding {
    .binding = 1,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment,
    .pImmutableSamplers = nullptr
  };
  vk::DescriptorSetLayoutBinding metallic_binding {
    .binding = 2,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment,
    .pImmutableSamplers = nullptr
  };
  vk::DescriptorSetLayoutBinding ao_binding {
    .binding = 3,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment,
    .pImmutableSamplers = nullptr
  };
  vk::DescriptorSetLayoutBinding factors_binding {
    .binding = 4,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment,
    .pImmutableSamplers = nullptr
  };
  default_layout_ = descriptors.descriptor_allocator->create_descriptor_set_layouts({albedo_binding, normal_binding, metallic_binding, ao_binding, factors_binding});
}

void MaterialManager::load_material(Handle<Material> handle, Material& material) {
  //std::cout << "Attempting to load texture handle: " << material.base_texture_handle.value << std::endl;
  
  VulkanMaterial vk_material;
  vk_material.descriptor_set = descriptors_->descriptor_allocator->allocate_descriptor_set(default_layout_);
  
  const VulkanTexture& albedo = texture_manager_->get_texture(material.albedo_texture_handle);
  descriptors_->descriptor_writer->write_image(0, image_manager_->get_image_view(albedo.image_handle), sampler_manager_->get_sampler(albedo.sampler_desc), vk::ImageLayout::eShaderReadOnlyOptimal);
  
  const VulkanTexture& normal = texture_manager_->get_texture(material.normal_texture_handle);
  descriptors_->descriptor_writer->write_image(1, image_manager_->get_image_view(normal.image_handle), sampler_manager_->get_sampler(normal.sampler_desc), vk::ImageLayout::eShaderReadOnlyOptimal);

  const VulkanTexture& metallic_roughness = texture_manager_->get_texture(material.metallic_roughness_texture_handle);
  descriptors_->descriptor_writer->write_image(2, image_manager_->get_image_view(metallic_roughness.image_handle), sampler_manager_->get_sampler(metallic_roughness.sampler_desc), vk::ImageLayout::eShaderReadOnlyOptimal);

  const VulkanTexture& ao = texture_manager_->get_texture(material.ao);
  descriptors_->descriptor_writer->write_image(3, image_manager_->get_image_view(ao.image_handle), sampler_manager_->get_sampler(ao.sampler_desc), vk::ImageLayout::eShaderReadOnlyOptimal);

  MaterialFactors factors {
    .albedo = material.albedo_texture_factor,
    .normal = material.normal_texture_scale,
    .metallic = material.metallic_factor,
    .roughness = material.roughness_factor,
    .ao = material.ao_strength
  };

  vk::raii::Buffer factor_buffer = nullptr;
  vk::raii::DeviceMemory factor_buffer_mem = nullptr;
  utils::create_buffer(context_, sizeof(factors), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, factor_buffer, factor_buffer_mem);
  
  void* memory_mapped = factor_buffer_mem.mapMemory(0, sizeof(factors));
  memcpy(memory_mapped, &factors, sizeof(factors));
  factor_buffer_mem.unmapMemory();

  descriptors_->descriptor_writer->write_buffer(4, factor_buffer, 0, sizeof(factors));

  descriptors_->descriptor_writer->update_set(context_->device, vk_material.descriptor_set);

  materials_.try_emplace(handle, std::move(vk_material));
  material_buffers_.try_emplace(handle, std::move(factor_buffer));
  material_buffer_memory_.try_emplace(handle, std::move(factor_buffer_mem));
}

void MaterialManager::unload_material(Handle<Material> handle) {
  assert(handle.is_valid());
  materials_.erase(handle);
}