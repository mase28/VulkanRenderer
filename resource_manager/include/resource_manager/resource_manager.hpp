#pragma once

#include <assets/mesh.hpp>
#include <common/handles.hpp>
#include <common/id_generator.hpp>

#include <unordered_map>
#include <string>

template<typename Handle>
struct AssetDelta {
  std::vector<Handle> added;
  std::vector<Handle> deleted;
  std::vector<Handle> modified;
};

template<typename Handle, typename Asset>
struct Registry {
  IdGenerator id_generator;
  std::unordered_map<Handle, Asset> assets;
  AssetDelta<Handle> delta;
};

enum class ImageType {
  RGB,
  LINEAR
};

class ResourceManager {
public:
  ResourceManager(const char* resource_dir);
  std::string get_resource_dir() {return resource_dir_;}
  std::string get_resource_file_path(std::string file_name) {return resource_files_.at(file_name);}
  std::string get_resource_file_parent(std::string file_name) {return resource_files_.at(file_name).substr(0, resource_files_.at(file_name).size() - file_name.size());}

  [[nodiscard]] Handle<Mesh> add_mesh(Mesh&& mesh) {return add_to_registry(meshes_, mesh);}
  [[nodiscard]] Handle<Texture> add_texture(Texture&& texture) {return add_to_registry(textures_, texture);}
  [[nodiscard]] Handle<Material> add_material(Material&& material) {return add_to_registry(materials_, material);}
  [[nodiscard]] Handle<SamplerDesc> add_sampler(SamplerDesc&& sampler) {return add_to_registry(samplers_, sampler);}
  [[nodiscard]] Handle<Image> add_image(Image&& image) {return add_to_registry(images_, image);}

  [[nodiscard]] Handle<Texture> get_white_texture_handle() {return white_texture_handle_;}
  [[nodiscard]] Handle<Texture> get_base_normal_texture_handle() {return default_normal_texture_handle_;}
  [[nodiscard]] Handle<Texture> get_base_metallic_roughness_texture_handle() {return default_metallic_roughness_texture_handle_;}

  Mesh& get_mesh(Handle<Mesh> handle) {assert(handle.is_valid()); return meshes_.assets.at(handle);}
  Texture& get_texture(Handle<Texture> handle) {assert(handle.is_valid()); return textures_.assets.at(handle);}
  Material& get_material(Handle<Material> handle) {assert(handle.is_valid()); return materials_.assets.at(handle);}
  SamplerDesc& get_sampler(Handle<SamplerDesc> handle) {assert(handle.is_valid()); return samplers_.assets.at(handle);}
  Image& get_image(Handle<Image> handle) {assert(handle.is_valid()); return images_.assets.at(handle);}

  void unload_mesh(Handle<Mesh> handle) {assert(handle.is_valid()); remove_from_registry(meshes_, handle);}
  void unload_texture(Handle<Texture> handle) {assert(handle.is_valid()); remove_from_registry(textures_, handle);}
  void unload_material(Handle<Material> handle) {assert(handle.is_valid()); remove_from_registry(materials_, handle);}
  void unload_sampler(Handle<SamplerDesc> handle) {assert(handle.is_valid()); remove_from_registry(samplers_, handle);}
  void unload_image(Handle<Image> handle) {assert(handle.is_valid()); remove_from_registry(images_, handle);}

  AssetDelta<Handle<Mesh>> get_mesh_delta() {return get_asset_delta(meshes_.delta);}
  AssetDelta<Handle<Texture>> get_texture_delta() {return get_asset_delta(textures_.delta);}
  AssetDelta<Handle<Material>> get_material_delta() {return get_asset_delta(materials_.delta);}
  AssetDelta<Handle<SamplerDesc>> get_sampler_delta() {return get_asset_delta(samplers_.delta);}
  AssetDelta<Handle<Image>> get_image_delta() {return get_asset_delta(images_.delta);}

private:
  std::string resource_dir_;
  std::unordered_map<std::string, std::string> resource_files_;
  
  Registry<Handle<Mesh>, Mesh> meshes_;
  Registry<Handle<Texture>, Texture> textures_;
  Registry<Handle<Material>, Material> materials_;
  Registry<Handle<SamplerDesc>, SamplerDesc> samplers_;
  Registry<Handle<Image>, Image> images_;

  template<typename Handle, typename Asset>
  Handle add_to_registry(Registry<Handle, Asset>& registry, Asset& asset) {
    Handle handle = {.value = registry.id_generator.get_unique_id()};
    registry.assets.emplace(handle, std::move(asset));
    registry.delta.added.push_back(handle);
    return handle;
  }

  template<typename Handle, typename Asset>
  void remove_from_registry(Registry<Handle, Asset>& registry, Handle handle) {
    registry.id_generator.return_id(handle.value);
    registry.assets.erase(handle);
    registry.delta.deleted.push_back(handle);
  }

  template<typename Handle>
  AssetDelta<Handle> get_asset_delta(AssetDelta<Handle>& delta) {
    AssetDelta<Handle> copy = delta;
    delta.added.clear();
    delta.deleted.clear();
    delta.modified.clear();
    return copy;
  }

  Handle<Image> white_image_handle_;
  Handle<Image> default_normal_image_handle_;
  Handle<Image> default_metallic_roughness_image_handle_;
  Handle<Texture> white_texture_handle_;
  Handle<Texture> default_normal_texture_handle_;
  Handle<Texture> default_metallic_roughness_texture_handle_;
  void create_defaults();
};