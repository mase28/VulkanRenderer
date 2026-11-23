#pragma once

#include <common/handles.hpp>

#include <camera/camera.hpp>
#include <importers/gltf_loader.hpp>
#include <platform/platform.hpp>
#include <renderer/renderer.hpp>
#include <resource_manager/resource_manager.hpp>
#include <scene/scene.hpp>

#include <cstdint>
#include <unordered_map>

class UI;

class Engine {
public:
  Engine();
  ~Engine();

  void set_resource_dir(const char *resource_dir);
  void run();

  // Handle<Mesh> load_obj(const char* obj_file);
  // Handle<Texture> load_texture(const char* texture_file);
  std::unordered_map<std::string, Handle<MeshInstance>>
  load_gltf(const char *gltf_file);

  Handle<MeshInstance> add_instance(Handle<Mesh> mesh_handle,
                                    glm::vec3 position);
  // void add_texture_to_mesh(Handle<MeshInstance> mesh_handle, Handle<Texture>
  // texture_handle);

private:
  std::unique_ptr<Platform> platform_ = nullptr;
  std::unique_ptr<ResourceManager> resource_manager_ = nullptr;
  std::unique_ptr<Scene> scene_ = nullptr;
  std::unique_ptr<GltfLoader> gltf_loader_ = nullptr;
  std::unique_ptr<Camera> camera_ = nullptr;
  std::unique_ptr<Renderer> renderer_ = nullptr;
  std::unique_ptr<UI> ui_ = nullptr;
};
