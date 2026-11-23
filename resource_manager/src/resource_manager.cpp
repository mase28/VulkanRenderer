#include <resource_manager/resource_manager.hpp>

#include <glm/glm.hpp>

#include <iostream>
#include <filesystem>

ResourceManager::ResourceManager(const char* resource_dir) {
  std::cout << "Retrieved resource dir: " << resource_dir << std::endl;
  for (const auto& file_name : std::filesystem::directory_iterator(resource_dir)) {
    std::cout << "Found resource: " << file_name.path().filename().string() << std::endl;
    resource_files_[file_name.path().filename().string()] = file_name.path().string();
  }

  create_defaults();
}

void ResourceManager::create_defaults() {
  Image image {
    .pixels = {255, 255, 255, 255},
    .width = 1,
    .height = 1
  };
  white_image_handle_ = add_to_registry(images_, image);

  Texture texture {
    .image_handle = white_image_handle_
  };
  white_texture_handle_ = add_to_registry(textures_, texture);

  Image normal_image {
    .pixels = {128, 128, 255, 255},
    .width = 1,
    .height = 1
  };
  default_normal_image_handle_ = add_to_registry(images_, normal_image);

  Texture normal_texture {
    .image_handle = default_normal_image_handle_
  };
  default_normal_texture_handle_ = add_to_registry(textures_, normal_texture);

  Image metallic_roughness_image {
    .pixels = {0, 255, 255, 0},
    .width = 1,
    .height = 1
  };
  default_metallic_roughness_image_handle_ = add_to_registry(images_, metallic_roughness_image);

  Texture metallic_roughness_texture {
    .image_handle = default_metallic_roughness_image_handle_
  };
  default_metallic_roughness_texture_handle_ = add_to_registry(textures_, metallic_roughness_texture);
}
