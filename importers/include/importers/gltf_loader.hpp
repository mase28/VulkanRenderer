#pragma once

#include <assets/mesh.hpp>
#include <common/handles.hpp>

#include <vector>
#include <unordered_map>
#include <string>

class ResourceManager;
class Scene;

namespace fastgltf {
  struct Mesh;
  struct Image;
  struct Asset;
  struct Material;
  struct Sampler;
  struct Texture;
}

class GltfLoader {
public:
  GltfLoader(ResourceManager* resource_manager, Scene* scene);
  
  const std::unordered_map<std::string, Handle<MeshInstance>>& load_file(std::string file_name);
private:
  ResourceManager* resource_manager_;
  Scene* scene_;

  std::unordered_map<size_t, Handle<Mesh>> meshes_;
  std::unordered_map<size_t, Handle<Texture>> textures_;
  std::unordered_map<size_t, Handle<Image>> images_;
  std::unordered_map<size_t, Handle<Material>> materials_;
  std::unordered_map<size_t, Handle<SamplerDesc>> samplers_;

  std::unordered_map<std::string, Handle<MeshInstance>> instances_;
  std::unordered_map<std::string, int> num_instances_;

  void load_mesh(Mesh& mesh, fastgltf::Mesh& f_mesh, fastgltf::Asset& asset);
  void load_meshes(fastgltf::Asset& asset);

  void load_image(Image& image, fastgltf::Image& f_image, fastgltf::Asset& asset);
  void load_images(fastgltf::Asset& asset);

  void load_sampler(SamplerDesc& sampler, fastgltf::Sampler& f_sampler, fastgltf::Asset& asset);
  void load_samplers(fastgltf::Asset& asset);

  void load_texture(Texture& texture, fastgltf::Texture& f_texture, fastgltf::Asset& asset);
  void load_textures(fastgltf::Asset& asset);

  void load_material(Material& material, fastgltf::Material& f_material, fastgltf::Asset& asset);
  void load_materials(fastgltf::Asset& asset);

  void process_node(fastgltf::Asset& asset, size_t node_idx, glm::mat4 parent_matrix);
};