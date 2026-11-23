#include "resource_loader.hpp"
#include <resource_manager/resource_manager.hpp>
#include <mesh_manager.hpp>
#include "texture_manager.hpp"
#include "material_manager.hpp"
#include "image_manager.hpp"
#include "sampler_manager.hpp"

#include <iostream>

ResourceLoader::ResourceLoader(VulkanContext& context, CommandContext& command_context, Config::ResourceManagers& resource_loader_managers) : context_(&context), command_context_(&command_context){
  resource_manager_ = resource_loader_managers.resource_manager;
  mesh_manager_ = resource_loader_managers.mesh_manager;
  texture_manager_ = resource_loader_managers.texture_manager;
  material_manager_ = resource_loader_managers.material_manager;
  image_manager_ = resource_loader_managers.image_manager;
  sampler_manager_ = resource_loader_managers.sampler_manager;
  
  sync_resources();
}

void ResourceLoader::load_image(Handle<Image> handle, Image& image) {
  image_manager_->load_image(handle, image);
}

void ResourceLoader::load_sampler(Handle<SamplerDesc> handle, SamplerDesc& sampler) {
  sampler_manager_->add_sampler(handle, sampler);
}

void ResourceLoader::load_static_mesh(Handle<Mesh> handle, Mesh& mesh) {
  mesh_manager_->load_static_mesh(handle, mesh);
}

void ResourceLoader::load_texture(Handle<Texture> handle, Texture& texture) {
  texture_manager_->load_texture(handle, texture);
}

void ResourceLoader::load_material(Handle<Material> handle, Material& material) {
  material_manager_->load_material(handle, material);
}

void ResourceLoader::sync_resources() {
  sync_images();
  sync_samplers();
  sync_textures();
  sync_materials();
  sync_meshes();
}

void ResourceLoader::sync_images() {
  AssetDelta image_delta = resource_manager_->get_image_delta();
  for (auto& image_handle : image_delta.added) {
    load_image(image_handle, resource_manager_->get_image(image_handle));
  }
  for (auto& image_handle : image_delta.deleted) {
    //unload(mesh_id);
  }
  for (auto& image_handle : image_delta.modified) {
    // do something
  }
}

void ResourceLoader::sync_samplers() {
  AssetDelta sampler_delta = resource_manager_->get_sampler_delta();
  for (auto& sampler_handle : sampler_delta.added) {
    load_sampler(sampler_handle, resource_manager_->get_sampler(sampler_handle));
  }
  for (auto& sampler_handle : sampler_delta.deleted) {
    //unload(mesh_id);
  }
  for (auto& sampler_handle : sampler_delta.modified) {
    // do something
  }
}

void ResourceLoader::sync_meshes() {
  AssetDelta mesh_delta = resource_manager_->get_mesh_delta();
  for (auto& mesh_handle : mesh_delta.added) {
    //std::cout << "loading mesh: " << mesh_handle.value << std::endl;
    load_static_mesh(mesh_handle, resource_manager_->get_mesh(mesh_handle));
  }
  for (auto& mesh_handle : mesh_delta.deleted) {
    //unload(mesh_id);
  }
  for (auto& mesh_handle : mesh_delta.modified) {
    // do something
  }
}

void ResourceLoader::sync_textures() {
  AssetDelta texture_delta = resource_manager_->get_texture_delta();
  for (auto& texture_handle : texture_delta.added) {
    load_texture(texture_handle, resource_manager_->get_texture(texture_handle));
  }
  for (auto& texture_handle : texture_delta.deleted) {
    //unload(mesh_id);
  }
  for (auto& texture_handle : texture_delta.modified) {
    // do something
  }
}

void ResourceLoader::sync_materials() {
  AssetDelta material_delta = resource_manager_->get_material_delta();
  for (auto& material_handle : material_delta.added) {
    load_material(material_handle, resource_manager_->get_material(material_handle));
  }
  for (auto& materialhandle : material_delta.deleted) {
    //unload(mesh_id);
  }
  for (auto& material_handle : material_delta.modified) {
    // do something
  }
}