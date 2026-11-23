#pragma once

#include <assets/mesh.hpp>
#include <assets/math_types.hpp>
#include <common/handles.hpp>
#include <common/id_generator.hpp>

#include <cstdint>
#include <unordered_map>

struct InstanceDelta {
  std::vector<Handle<MeshInstance>> added;
  std::vector<Handle<MeshInstance>> deleted;
  std::vector<Handle<MeshInstance>> modified;
};

class Scene {
public:
  //Scene();
  Handle<MeshInstance> add_instance(Handle<Mesh> mesh_handle, glm::vec3 position);
  Handle<MeshInstance> add_instance(Handle<Mesh> mesh_handle, glm::mat4 model_matrix);
  void delete_instance(Handle<MeshInstance> mesh_instance_handle);

  //void add_texture_to_mesh(Handle<MeshInstance> mesh_instance_handle, Handle<Texture> texture_handle);

  const MeshInstance& get_instance(Handle<MeshInstance> mesh_instance_handle) {return instances_.at(mesh_instance_handle);}
  const std::unordered_map<Handle<MeshInstance>, MeshInstance>& get_instances() {return instances_;}

  InstanceDelta get_delta();

  // void move_instance(uint32_t instance_id, Position position);
  // void rotate_instance(uint32_t instance_id, )
private:
  std::unordered_map<Handle<MeshInstance>, MeshInstance> instances_;

  InstanceDelta delta_;

  IdGenerator id_generator_;
};