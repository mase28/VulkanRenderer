#include "ui.hpp"
#include <assets/mesh.hpp>
#include <engine/engine.hpp>

#include <imgui.h>

#include <array>
#include <iostream>
#include <string>

Engine::Engine() {
  platform_ = std::make_unique<Platform>();
  scene_ = std::make_unique<Scene>();
  camera_ = std::make_unique<Camera>(glm::vec3{0.0f, 0.0f, -1.0f});
  ui_ = std::make_unique<UI>(scene_.get());
}

Engine::~Engine() { std::cout << "Engine destroyed\n"; }

void Engine::set_resource_dir(const char *resource_dir) {
  resource_manager_ = std::make_unique<ResourceManager>(resource_dir);
  gltf_loader_ =
      std::make_unique<GltfLoader>(resource_manager_.get(), scene_.get());
}

// Handle<Mesh> Engine::load_obj(const char* obj_file) {
//   return resource_manager_->load_obj(obj_file);
// }

// Handle<Texture> Engine::load_texture(const char* texture_file) {
//   return resource_manager_->load_texture(texture_file);
// }

std::unordered_map<std::string, Handle<MeshInstance>>
Engine::load_gltf(const char *gltf_file) {
  std::unordered_map<std::string, Handle<MeshInstance>> instances =
      gltf_loader_->load_file(gltf_file);
  return instances;
}

Handle<MeshInstance> Engine::add_instance(Handle<Mesh> mesh_handle,
                                          glm::vec3 position) {
  assert(mesh_handle.is_valid());
  return scene_->add_instance(mesh_handle, position);
}

// void Engine::add_texture_to_mesh(Handle<MeshInstance> mesh_handle,
// Handle<Texture> texture_handle) {
//   assert(mesh_handle.is_valid() && texture_handle.is_valid());
//   scene_->add_texture_to_mesh(mesh_handle, texture_handle);
// }

void Engine::run() {
  VulkanRendererConfig config{
      .max_frames_in_flight = 2,
      .width = 1600,
      .height = 800,
      .extensions = platform_->get_window_extensions(),
      .window_providers = platform_->get_window_providers(),
      .camera = camera_.get(),
      .scene = scene_.get(),
      .resource_manager = resource_manager_.get(),
  };
  renderer_ = create_renderer(config);
  renderer_->init();

  std::array<float, 3> float_test = {0.0f, 0.0f, 0.0f};
  auto test = [&float_test]() { ImGui::ColorEdit3("test", float_test.data()); };
  ui_->add_window("test", test);

  while (true) {
    platform_->poll();
    if (platform_->should_close()) {
      break;
    }
    float delta_time = renderer_->get_delta_time();
    glm::vec3 input = platform_->get_input();
    // if (glm::length(input) != 0) {
    //   std::cout << "Input received: x=" << input[0] << ", y=" << input[1] <<
    //   ", z=" << input[2] << std::endl;
    // }
    glm::vec2 mouse_location = platform_->get_mouse_delta();
    // std::cout << "mouse_location: x=" << mouse_location[0] << ", y=" <<
    // mouse_location[1] << std::endl;
    camera_->look_at(mouse_location);
    camera_->move_direction(input, delta_time);

    // new frame platform needs to go first for imgui
    platform_->new_frame();
    renderer_->begin_frame();
    ui_->draw();
    renderer_->draw_frame();
  }
  renderer_->wait();
}
