#pragma once

#include <renderer_types.hpp>
#include <assets/mesh.hpp>
#include <common/id_generator.hpp>
#include <common/handles.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

class ImageManager;
class SamplerManager;

class TextureManager {
public:
  TextureManager(ImageManager* image_manager, SamplerManager* sampler_manager);

  void load_texture(Handle<Texture> handle, Texture& texture);
  void unload_texture(Handle<Texture> handle);

  const VulkanTexture& get_texture(Handle<Texture> handle) {assert(handle.is_valid()); return textures_.at(handle);}
private:
  ImageManager* image_manager_;
  SamplerManager* sampler_manager_;

  std::unordered_map<Handle<Texture>, VulkanTexture> textures_;
};