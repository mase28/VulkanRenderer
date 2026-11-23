#pragma once

#include <scene/scene.hpp>
#include <common/handles.hpp>
#include <assets/mesh.hpp>

#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

#include <unordered_map>

class ResourceLoader;
class MaterialManager;

struct Descriptors;

class InstanceManager {
public:
  InstanceManager(VulkanContext& context, Descriptors& descriptors, Scene* scene, Config::ResourceManagers& resource_loader_managers);


  // TODO: allow instance manager to use new managers to create instances
  
  void sync_instances();

  const std::unordered_map<Handle<MeshInstance>, VulkanMeshInstance>& get_instances();
private:
  VulkanContext* context_;
  Descriptors* descriptors_;
  ResourceLoader* resource_loader_;
  Scene* scene_;
  MeshManager* mesh_manager_;
  TextureManager* texture_manager_;
  MaterialManager* material_manager_;

  std::unordered_map<Handle<MeshInstance>, VulkanMeshInstance> instances_;

  void sync_mesh_instances();

  void create_instance(const Handle<MeshInstance>& handle, const MeshInstance& mesh_instance);
  void delete_instance(Handle<MeshInstance> mesh_handle);

  void create_transforms(VulkanMeshInstance& mesh_instance, glm::mat4 model_matrix);
};