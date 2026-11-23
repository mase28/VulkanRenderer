#include "instance_manager.hpp"
#include "utils.hpp"
#include "descriptors.hpp"
#include "mesh_manager.hpp"
#include "texture_manager.hpp"
#include "material_manager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

InstanceManager::InstanceManager(VulkanContext& context, Descriptors& descriptors, Scene* scene, Config::ResourceManagers& resource_loader_managers) 
  : context_(&context), descriptors_(&descriptors), scene_(scene) {
  mesh_manager_ = resource_loader_managers.mesh_manager;
  texture_manager_ = resource_loader_managers.texture_manager;
  material_manager_ = resource_loader_managers.material_manager;
  sync_instances();
};

void InstanceManager::sync_instances() {
  sync_mesh_instances();
}

void InstanceManager::sync_mesh_instances() {
  InstanceDelta delta = scene_->get_delta();
  for (auto& mesh_instance_handle : delta.added) {
    create_instance(mesh_instance_handle, scene_->get_instance(mesh_instance_handle));
  }
  for(auto& mesh_instance_handle : delta.deleted) {
    instances_.erase(mesh_instance_handle);
  }
  for(auto& instance_id : delta.modified) {
    // will change for when you can update instances
    continue;
  }
}

const std::unordered_map<Handle<MeshInstance>, VulkanMeshInstance>& InstanceManager::get_instances() {
  sync_instances();
  return instances_;
}

void InstanceManager::create_instance(const Handle<MeshInstance>& handle, const MeshInstance& mesh_instance) {
  //std::cout << "Attempting to create instance with mesh id: " << mesh_instance.mesh_handle.value << std::endl;
  const RenderStaticMesh& mesh = mesh_manager_->get_static_mesh(mesh_instance.mesh_handle);
  VulkanMeshInstance vulkan_mesh_instance;
  vulkan_mesh_instance.mesh_handle = mesh_instance.mesh_handle;
  create_transforms(vulkan_mesh_instance, mesh_instance.model_matrix);
  
  instances_[handle] = std::move(vulkan_mesh_instance);
}

void InstanceManager::create_transforms(VulkanMeshInstance& mesh_instance, glm::mat4 model_matrix) {
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory buffer_memory = nullptr;
  utils::create_buffer(context_, sizeof(model_matrix) * context_->max_frames_in_flight, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, buffer, buffer_memory);

  TransformBuffer transform_buffer {
    .model_matrix = std::vector<glm::mat4>(context_->max_frames_in_flight, model_matrix),
    .buffer = std::move(buffer),
    .memory = std::move(buffer_memory)
  };
  transform_buffer.memory_mapped = transform_buffer.memory.mapMemory(0, sizeof(model_matrix) * context_->max_frames_in_flight);
  
  for (int i = 0; i < context_->max_frames_in_flight; i++) {
    memcpy(static_cast<std::byte*>(transform_buffer.memory_mapped) + (sizeof(model_matrix) * i), &model_matrix, sizeof(model_matrix));
  }

  mesh_instance.transform_buffer = std::move(transform_buffer);
}

