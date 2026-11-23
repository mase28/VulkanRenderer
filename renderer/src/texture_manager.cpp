#include "texture_manager.hpp"
#include "image_manager.hpp"
#include "sampler_manager.hpp"

#include <iostream>
#include <cstdint>
#include <cmath>

TextureManager::TextureManager(ImageManager* image_manager, SamplerManager* sampler_manager) : image_manager_(image_manager), sampler_manager_(sampler_manager) {
}

void TextureManager::load_texture(Handle<Texture> handle, Texture& texture) {
  VulkanTexture vk_texture;
  vk_texture.image_handle = texture.image_handle;
  if (texture.has_sampler) {
    vk_texture.sampler_desc = texture.sampler_handle;
  } else {
    vk_texture.sampler_desc = sampler_manager_->get_default_sampler_handle();
  }
  textures_.try_emplace(handle, std::move(vk_texture));
}

void TextureManager::unload_texture(Handle<Texture> handle) {
  assert(handle.is_valid());
  textures_.erase(handle);
}
