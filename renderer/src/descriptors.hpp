#pragma once

#include "renderer_types.hpp"
#include <common/id_generator.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>
#include <vector>
#include <cstdint>

// Add different layout types here. Future layouts: Frame global such as camera
enum class DescriptorLayoutType {
  FrameGlobals,
  MeshInstanceTransform,
  Material
};

class DescriptorAllocator {
public:
  DescriptorAllocator(VulkanContext& context);

  Handle<vk::DescriptorSetLayout> create_descriptor_set_layouts(std::vector<vk::DescriptorSetLayoutBinding> layout_bindings);

  vk::DescriptorSet allocate_descriptor_set(Handle<vk::DescriptorSetLayout> handle);

  vk::DescriptorSetLayout get_layout(Handle<vk::DescriptorSetLayout> handle) {return layouts_.at(handle);}
private:
  VulkanContext* context_;
  std::unordered_map<Handle<vk::DescriptorSetLayout>, vk::raii::DescriptorSetLayout> layouts_;
  std::unordered_map<Handle<vk::DescriptorSetLayout>, vk::raii::DescriptorPool> pools_;
  std::unordered_map<Handle<vk::DescriptorSetLayout>, std::vector<vk::raii::DescriptorSet>> owned_sets_;

  bool create_descriptor_pool(Handle<vk::DescriptorSetLayout> handle, uint32_t num_objects, uint32_t num_sets);

  IdGenerator id_generator_;
};

class DescriptorWriter {
public:
  void write_image(uint32_t binding, vk::ImageView image_view, vk::Sampler sampler, vk::ImageLayout image_layout);
  void write_buffer(uint32_t binding, vk::Buffer buffer, uint32_t offset, size_t range);
  void update_set(const vk::raii::Device& device, vk::DescriptorSet descriptor_set);
private:
  std::unordered_map<uint32_t, vk::DescriptorImageInfo> image_infos_;
  std::unordered_map<uint32_t, vk::DescriptorBufferInfo> buffer_infos_;
};