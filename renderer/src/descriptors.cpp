#include "descriptors.hpp"
#include "vulkan/vulkan.hpp"

#include <iostream>
#include <array>

DescriptorAllocator::DescriptorAllocator(VulkanContext& context) : context_(&context) {
}

Handle<vk::DescriptorSetLayout> DescriptorAllocator::create_descriptor_set_layouts(std::vector<vk::DescriptorSetLayoutBinding> layout_binds) {

  vk::DescriptorSetLayoutCreateInfo create_info {
    .bindingCount = static_cast<uint32_t>(layout_binds.size()),
    .pBindings = layout_binds.data()
  };
  Handle<vk::DescriptorSetLayout> handle = {id_generator_.get_unique_id()};
  layouts_.try_emplace(handle, context_->device, create_info);
  return handle;
}

bool DescriptorAllocator::create_descriptor_pool(Handle<vk::DescriptorSetLayout> handle, uint32_t num_objects, uint32_t num_sets) {

  // material contains 4 textures and thus needs 4 descriptor sets for each material object estimated ~ 30
  // TODO add uniform buffer for texture factors:
 
  vk::DescriptorPoolSize textures_size {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = num_objects * num_sets};
  vk::DescriptorPoolSize factors_size {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = num_objects};
  std::array<vk::DescriptorPoolSize, 2> pool_sizes {textures_size, factors_size};

  vk::DescriptorPoolCreateInfo material_create_info {
    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets = num_objects,
    .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
    .pPoolSizes = pool_sizes.data()
  };
  pools_.try_emplace(handle, context_->device, material_create_info);
  
  if (pools_.at(handle) == nullptr) {
    std::cout << "Unable to create descriptor pool\n";
    return false;
  } 
  return true;
}

vk::DescriptorSet DescriptorAllocator::allocate_descriptor_set(Handle<vk::DescriptorSetLayout> handle) {
  if (pools_.find(handle) == pools_.end()) {
    create_descriptor_pool(handle, 30, 4);
  }
  vk::DescriptorSetAllocateInfo alloc_info {
    .descriptorPool = pools_.at(handle),
    .descriptorSetCount = 1,
    .pSetLayouts = &*layouts_.at(handle)
  };

  vk::raii::DescriptorSet descriptor_set = std::move(context_->device.allocateDescriptorSets(alloc_info).front());
  vk::DescriptorSet descriptor_handle = *descriptor_set;
  if (owned_sets_.find(handle) == owned_sets_.end()) {
    std::vector<vk::raii::DescriptorSet> sets;
    sets.push_back(std::move(descriptor_set));
    owned_sets_.try_emplace(handle, std::move(sets));
  } else {
    owned_sets_.at(handle).push_back(std::move(descriptor_set));
  }

  return descriptor_handle;
}

void DescriptorWriter::write_image(uint32_t binding, vk::ImageView image_view, vk::Sampler sampler, vk::ImageLayout image_layout) {
  vk::DescriptorImageInfo image_info {
    .sampler = sampler,
    .imageView = image_view,
    .imageLayout = image_layout
  };
  image_infos_.try_emplace(binding, image_info);
}

void DescriptorWriter::write_buffer(uint32_t binding, vk::Buffer buffer, uint32_t offset, size_t range) {
  vk::DescriptorBufferInfo buffer_info {
    .buffer = buffer,
    .offset = offset,
    .range = range
  };
  buffer_infos_.try_emplace(binding, buffer_info);
}

void DescriptorWriter::update_set(const vk::raii::Device& device, vk::DescriptorSet descriptor_set) {
  for (auto& [binding, image_info] : image_infos_) {
    vk::WriteDescriptorSet descriptor_write {
      .dstSet = descriptor_set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .pImageInfo = &image_info
    };
    device.updateDescriptorSets(descriptor_write, {});
  }
  image_infos_.clear();

  for (auto& [binding, buffer_info] : buffer_infos_) {
    vk::WriteDescriptorSet descriptor_write {
      .dstSet = descriptor_set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .pBufferInfo = &buffer_info
    };
    device.updateDescriptorSets(descriptor_write, {});
  }
  buffer_infos_.clear();
}
