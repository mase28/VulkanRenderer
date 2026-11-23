#pragma once

#include "renderer_types.hpp"
#include <common/handles.hpp>
#include <common/id_generator.hpp>
#include <assets/mesh.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

class MeshManager {
public:
  MeshManager(VulkanContext& context, CommandContext& command_context);

  void load_static_mesh(Handle<Mesh> handle, Mesh& mesh);
  void unload_static_mesh(Handle<Mesh> handle);

  const RenderStaticMesh& get_static_mesh(Handle<Mesh> handle) {return static_meshes_.at(handle);}
private:
  VulkanContext* context_;
  CommandContext* command_context_;

  std::unordered_map<Handle<Mesh>, RenderStaticMesh> static_meshes_;

  void copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size, vk::PipelineStageFlags2 dst_stage_mask, vk::AccessFlags2 dst_access_mask);

  VertexBuffer create_vertex_buffer(const Mesh& mesh);
  IndexBuffer create_index_buffer(const std::vector<uint32_t>& indices);
};
