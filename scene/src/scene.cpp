#include <scene/scene.hpp>

#include <glm/gtc/matrix_transform.hpp>

Handle<MeshInstance> Scene::add_instance(Handle<Mesh> mesh_handle, glm::vec3 position) {
  MeshInstance mesh_instance {
    .mesh_handle = mesh_handle,
    .model_matrix = glm::translate(glm::mat4(1.0f), position)
  };
  Handle<MeshInstance> handle {id_generator_.get_unique_id()};
  instances_[handle] = mesh_instance;
  delta_.added.push_back(handle);
  return handle;
}

Handle<MeshInstance> Scene::add_instance(Handle<Mesh> mesh_handle, glm::mat4 model_matrix) {
  MeshInstance mesh_instance {
    .mesh_handle = mesh_handle,
    .model_matrix = model_matrix
  };
  Handle<MeshInstance> handle {id_generator_.get_unique_id()};
  instances_[handle] = mesh_instance;
  delta_.added.push_back(handle);
  return handle;
}

// void Scene::add_texture_to_mesh(Handle<MeshInstance> mesh_instance_handle, Handle<Texture> texture_handle) {
//   instances_.at(mesh_instance_handle).texture_handle = texture_handle;
// }

void Scene::delete_instance(Handle<MeshInstance> mesh_instance_handle) {
  instances_.erase(mesh_instance_handle);
  id_generator_.return_id(mesh_instance_handle.value);
  delta_.deleted.push_back(mesh_instance_handle);
}

InstanceDelta Scene::get_delta() {
  InstanceDelta delta = delta_;
  delta_.added.clear();
  delta_.deleted.clear();
  delta_.modified.clear();
  return delta;
}
