#pragma once

#include <assets/mesh.hpp>
#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

class ResourceManager;
class MeshManager;
class TextureManager;
class MaterialManager;
class ImageManager;
class SamplerManager;

class ResourceLoader {
public:
  ResourceLoader(VulkanContext& context, CommandContext& command_context, Config::ResourceManagers& resource_loader_managers);

  void sync_resources();
private:
  VulkanContext* context_;
  CommandContext* command_context_;
  ResourceManager* resource_manager_;

  MeshManager* mesh_manager_;
  TextureManager* texture_manager_;
  MaterialManager* material_manager_;
  ImageManager* image_manager_;
  SamplerManager* sampler_manager_;

  void sync_meshes();
  void sync_textures();
  void sync_materials();
  void sync_images();
  void sync_samplers();

  void load_static_mesh(Handle<Mesh> handle, Mesh& mesh);
  void load_texture(Handle<Texture> handle, Texture& texture);
  void load_material(Handle<Material> handle, Material& material);
  void load_image(Handle<Image> handle, Image& image);
  void load_sampler(Handle<SamplerDesc> handle, SamplerDesc& sampler);
};