#include <importers/gltf_loader.hpp>
#include <scene/scene.hpp>
#include <resource_manager/resource_manager.hpp>
#include "tangent_calc.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <stb_image.h>

#include <iostream>
#include <variant>
#include <cstdint>
#include <numeric>

GltfLoader::GltfLoader(ResourceManager* resource_manager, Scene* scene) {
  assert(resource_manager != nullptr && scene != nullptr);

  resource_manager_ = resource_manager;
  scene_ = scene;
}

const std::unordered_map<std::string, Handle<MeshInstance>>& GltfLoader::load_file(std::string file_name) {
  constexpr auto options = fastgltf::Options::AllowDouble |
                 fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                 fastgltf::Options::GenerateMeshIndices;

  std::cout << "File path: " << resource_manager_->get_resource_file_path(file_name) << std::endl;
  auto data_buffer = fastgltf::GltfDataBuffer::FromPath(resource_manager_->get_resource_file_path(file_name));
  fastgltf::Parser parser;

  fastgltf::Expected<fastgltf::Asset> asset = parser.loadGltf(data_buffer.get(), resource_manager_->get_resource_file_parent(file_name), options);
  auto error_code = asset.error();
  if (error_code != fastgltf::Error::None) {
    std::cout << "Error with gltf file loading: " << fastgltf::getErrorMessage(error_code) << std::endl;
    throw std::runtime_error("Error loading glft file");
  }
  load_images(asset.get());
  load_samplers(asset.get());
  load_textures(asset.get());
  load_materials(asset.get());
  load_meshes(asset.get());

  auto& scene = asset->scenes[asset->defaultScene.value_or(0)];
  for (auto node_idx : scene.nodeIndices) {
    process_node(asset.get(), node_idx, glm::mat4(1.0f));
  }

  return instances_;
}

void GltfLoader::load_mesh(Mesh& mesh, fastgltf::Mesh& f_mesh, fastgltf::Asset& asset) {
  mesh.primitives.resize(f_mesh.primitives.size());
  for (auto it = f_mesh.primitives.begin(); it != f_mesh.primitives.end(); it++) {
    auto position_it = it->findAttribute("POSITION");
    assert(position_it != it->attributes.end());
    assert(it->indicesAccessor.has_value());

    Primitive primitive;

    auto& pos_accessor = asset.accessors[position_it->accessorIndex];

    size_t vertex_offset = mesh.vertex_pos.size();
    mesh.vertex_pos.resize(vertex_offset + pos_accessor.count);
    fastgltf::copyFromAccessor<glm::vec3>(asset, pos_accessor, mesh.vertex_pos.data() + vertex_offset);

    std::size_t tex_coord_idx = 0;
    if (it->materialIndex.has_value()) {
      auto& material = asset.materials[*it->materialIndex];
      primitive.material_handle = materials_.at(*it->materialIndex);
      if (material.pbrData.baseColorTexture.has_value()) {
        tex_coord_idx = material.pbrData.baseColorTexture->texCoordIndex;
      }
    }

    auto tex_coord_attribute = std::string("TEXCOORD_") + std::to_string(tex_coord_idx);
    auto tex_coord = it->findAttribute(tex_coord_attribute);
    if (tex_coord != it->attributes.end()) {
      auto& tex_coord_accessor = asset.accessors[tex_coord->accessorIndex];

      size_t tex_coord_offset = mesh.vertex_tex_coord.size();
      mesh.vertex_tex_coord.resize(tex_coord_offset + tex_coord_accessor.count);
      fastgltf::copyFromAccessor<glm::vec2>(asset, tex_coord_accessor, mesh.vertex_tex_coord.data() + tex_coord_offset);
    }

    auto normal_it = it->findAttribute("NORMAL");
    assert(normal_it != it->attributes.end());

    auto& norm_accessor = asset.accessors[normal_it->accessorIndex];

    size_t normal_offset = mesh.vertex_normal.size();
    mesh.vertex_normal.resize(normal_offset + norm_accessor.count);
    fastgltf::copyFromAccessor<glm::vec3>(asset, norm_accessor, mesh.vertex_normal.data() + normal_offset);

    auto& idx_accessor = asset.accessors[*it->indicesAccessor];

    size_t index_offset = mesh.indices.size();
    mesh.indices.resize(index_offset + idx_accessor.count);
    fastgltf::copyFromAccessor<uint32_t>(asset, idx_accessor, mesh.indices.data() + index_offset);

    // add offset to indices to get actual vertex buffer index
    for (auto it = mesh.indices.begin() + index_offset; it != mesh.indices.end(); it++) {
      *it += static_cast<uint32_t>(vertex_offset);
    }

    primitive.first_index = static_cast<uint32_t>(index_offset);
    primitive.index_count = static_cast<uint32_t>(idx_accessor.count);

    // Calc tangents if missing from gltf file, do after loading everything else
    mesh.vertex_tangent.resize(vertex_offset + pos_accessor.count);
    auto tangent_it = it->findAttribute("TANGENT");
    if (tangent_it != it->attributes.end()) {
      auto& tangent_accessor = asset.accessors[tangent_it->accessorIndex];
      fastgltf::copyFromAccessor<glm::vec4>(asset, tangent_accessor, mesh.vertex_tangent.data() + vertex_offset);
    } else {
      TangentCalc::calc_tangents(mesh);
    }
    mesh.primitives[std::distance(f_mesh.primitives.begin(), it)] = std::move(primitive);
  }
}

void GltfLoader::load_meshes(fastgltf::Asset& asset) {
  for (auto it = asset.meshes.begin(); it != asset.meshes.end(); it++) {
    Mesh mesh;
    load_mesh(mesh, *it, asset);
    meshes_.try_emplace(std::distance(asset.meshes.begin(), it), resource_manager_->add_mesh(std::move(mesh)));
  }
}

void GltfLoader::load_image(Image& image, fastgltf::Image& f_image, fastgltf::Asset& asset) {
  std::visit(fastgltf::visitor {
    [&](auto& arg) {throw std::runtime_error("Gltf image type not supported");},
    [&](fastgltf::sources::URI& file) {
      assert(file.fileByteOffset == 0);
      assert(file.uri->isLocalPath()); // only capable of loading local files

      int width, height, channels;
      const std::string path(file.uri->path().begin(), file.uri->path().end());
      stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

      image.pixels = std::vector<uint8_t> (pixels, pixels + width * height * 4);
      image.width = width;
      image.height = height;

      stbi_image_free(pixels);
    },
    [&](fastgltf::sources::Array& array) {
      int width, height, channels;
      stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(array.bytes.data()), static_cast<int>(array.bytes.size()), &width, &height, &channels, STBI_rgb_alpha);

      image.pixels = std::vector<uint8_t> (pixels, pixels + width * height * 4);
      image.width = width;
      image.height = height;

      stbi_image_free(pixels);
    },
    [&](fastgltf::sources::Vector& vector) {
      int width, height, channels;
      stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &channels, STBI_rgb_alpha);

      image.pixels = std::vector<uint8_t> (pixels, pixels + width * height * 4);
      image.width = width;
      image.height = height;

      stbi_image_free(pixels);
    },
    [&](fastgltf::sources::BufferView& view) {
      auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
      auto& buffer = asset.buffers[buffer_view.bufferIndex];

      std::visit(fastgltf::visitor {
        [&](auto& arg) {throw std::runtime_error("Gltf image type not supported");},
        [&](fastgltf::sources::Array& array) {
          int width, height, channels;
          stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(array.bytes.data()), static_cast<int>(array.bytes.size()), &width, &height, &channels, STBI_rgb_alpha);

          image.pixels = std::vector<uint8_t> (pixels, pixels + width * height * 4);
          image.width = width;
          image.height = height;

          stbi_image_free(pixels);
        },
        [&](fastgltf::sources::Vector& vector) {
          int width, height, channels;
          stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()), &width, &height, &channels, STBI_rgb_alpha);

          image.pixels = std::vector<uint8_t> (pixels, pixels + width * height * 4);
          image.width = width;
          image.height = height;

          stbi_image_free(pixels);
        }
      }, buffer.data);
    }
  }, f_image.data);
}

void GltfLoader::load_images(fastgltf::Asset& asset) {
  for (auto it = asset.images.begin(); it != asset.images.end(); it++) {
    Image image;
    load_image(image, *it, asset);
    images_.try_emplace(std::distance(asset.images.begin(), it), resource_manager_->add_image(std::move(image)));
  }
}

void GltfLoader::load_sampler(SamplerDesc& sampler, fastgltf::Sampler& f_sampler, fastgltf::Asset& asset) {
  if (f_sampler.magFilter.has_value()) {
    switch (f_sampler.magFilter.value()) {
      case fastgltf::Filter::Linear:
        sampler.mag_filter = SamplerConfig::MagFilter::LINEAR;
        break;
      case fastgltf::Filter::Nearest:
        sampler.mag_filter = SamplerConfig::MagFilter::NEAREST;
        break;
      default:
        sampler.mag_filter = SamplerConfig::MagFilter::LINEAR;
        break;
    }
  } else {
    sampler.mag_filter = SamplerConfig::MagFilter::LINEAR;
  }

  if (f_sampler.minFilter.has_value()) {
    switch (f_sampler.minFilter.value()) {
      case fastgltf::Filter::Linear:
        sampler.min_filter = SamplerConfig::MinFilter::LINEAR;
        break;
      case fastgltf::Filter::Nearest:
        sampler.min_filter = SamplerConfig::MinFilter::NEAREST;
        break;
      case fastgltf::Filter::NearestMipMapNearest:
        sampler.min_filter = SamplerConfig::MinFilter::NEAREST_MIPMAP_NEAREST;
        break;
      case fastgltf::Filter::NearestMipMapLinear:
        sampler.min_filter = SamplerConfig::MinFilter::NEAREST_MIPMAP_LINEAR;
        break;
      case fastgltf::Filter::LinearMipMapNearest:
        sampler.min_filter = SamplerConfig::MinFilter::LINEAR_MIPMAP_NEAREST;
        break;
      case fastgltf::Filter::LinearMipMapLinear:
        sampler.min_filter = SamplerConfig::MinFilter::LINEAR_MIPMAP_LINEAR;
        break;
    }
  } else {
    sampler.min_filter = SamplerConfig::MinFilter::LINEAR;
  }

  // fastgltf docs say only repeat for sampler wrapS and wrapT
  sampler.wrap_s = SamplerConfig::WrapS::REPEAT;
  sampler.wrap_t = SamplerConfig::WrapT::REPEAT;
}

void GltfLoader::load_samplers(fastgltf::Asset& asset) {
  for (auto it = asset.samplers.begin(); it != asset.samplers.end(); it++) {
    SamplerDesc sampler;
    load_sampler(sampler, *it, asset);
    samplers_.try_emplace(std::distance(asset.samplers.begin(), it), resource_manager_->add_sampler(std::move(sampler)));
  }
}

void GltfLoader::load_texture(Texture& texture, fastgltf::Texture& f_texture, fastgltf::Asset& asset) {

  texture.image_handle = images_.at(f_texture.imageIndex.value());

  if (f_texture.samplerIndex.has_value()) {
    texture.has_sampler = true;
    texture.sampler_handle = samplers_.at(f_texture.samplerIndex.value());
  }
}

void GltfLoader::load_textures(fastgltf::Asset& asset) {
  for (auto it = asset.textures.begin(); it != asset.textures.end(); it++) {
    if (!it->imageIndex.has_value()) {
      continue;
    }
    Texture texture;
    load_texture(texture, *it, asset);
    textures_.try_emplace(std::distance(asset.textures.begin(), it), resource_manager_->add_texture(std::move(texture)));
  }
}

void GltfLoader::load_material(Material& material, fastgltf::Material& f_material, fastgltf::Asset& asset) {

  if (f_material.pbrData.baseColorTexture.has_value()) {
    material.albedo_texture_handle = textures_.at(f_material.pbrData.baseColorTexture->textureIndex);
  } else {
    material.albedo_texture_handle = resource_manager_->get_white_texture_handle();
  }
  auto& albedo_factor = f_material.pbrData.baseColorFactor;
  material.albedo_texture_factor = {albedo_factor.x(), albedo_factor.y(), albedo_factor.z(), albedo_factor.w()};

  if (f_material.normalTexture.has_value()) {
    material.normal_texture_handle = textures_.at(f_material.normalTexture->textureIndex);
    material.normal_texture_scale = f_material.normalTexture->scale;
  } else {
    material.normal_texture_handle = resource_manager_->get_base_normal_texture_handle();
    material.normal_texture_scale = 1.0f;
  }

  if (f_material.pbrData.metallicRoughnessTexture.has_value()) {
    material.metallic_roughness_texture_handle = textures_.at(f_material.pbrData.metallicRoughnessTexture->textureIndex);
  } else {
    material.metallic_roughness_texture_handle = resource_manager_->get_base_metallic_roughness_texture_handle();
  }
  material.metallic_factor = f_material.pbrData.metallicFactor;
  material.roughness_factor = f_material.pbrData.roughnessFactor;

  if (f_material.occlusionTexture.has_value()) {
    material.ao = textures_.at(f_material.occlusionTexture->textureIndex);
    material.ao_strength = f_material.occlusionTexture->strength;
  } else {
    material.ao = resource_manager_->get_white_texture_handle();
    material.ao_strength = 1.0;
  }
}

void GltfLoader::load_materials(fastgltf::Asset& asset) {
  for (auto it = asset.materials.begin(); it != asset.materials.end(); it++) {
    Material material;
    load_material(material, *it, asset);
    materials_[std::distance(asset.materials.begin(), it)] = resource_manager_->add_material(std::move(material));
  }
}

void GltfLoader::process_node(fastgltf::Asset& asset, size_t node_idx, glm::mat4 parent_matrix) {

  auto& node = asset.nodes[node_idx];

  glm::mat4 node_matrix(1.0f);
  std::visit(fastgltf::visitor {
    [&](auto& arg) {},
    [&](fastgltf::TRS& trs) {
      node_matrix = glm::translate(node_matrix, {trs.translation.x(), trs.translation.y(), trs.translation.z()});
      node_matrix = node_matrix * glm::toMat4(glm::quat(trs.rotation.w(), trs.rotation.x(), trs.rotation.y(), trs.rotation.z()));
      node_matrix = glm::scale(node_matrix, {trs.scale.x(), trs.scale.y(), trs.scale.z()});
    },
    [&](fastgltf::math::fmat4x4& mat) {
      for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
          node_matrix[col][row] = mat[col][row];
        }
      }
    }
  }, node.transform);

  glm::mat4 world_matrix = parent_matrix * node_matrix;

  if (node.meshIndex.has_value()) {
    Handle<Mesh> mesh_handle = meshes_.at(*node.meshIndex);
    Handle<MeshInstance> instance_handle = scene_->add_instance(mesh_handle, world_matrix);

    auto& f_mesh = asset.meshes[*node.meshIndex];
    if (auto instance_idx = instances_.find(f_mesh.name.data()); instance_idx != instances_.end()) {
      if (auto num_instance_idx = num_instances_.find(instance_idx->first); num_instance_idx != num_instances_.end()) {
        int instance_count = num_instances_.at(instance_idx->first);
        num_instances_.at(instance_idx->first) = instance_count + 1;
        instances_[f_mesh.name.data() + std::to_string(instance_count)] = instance_handle;
      } else {
        num_instances_[instance_idx->first] = 2;
        instances_[f_mesh.name.data() + std::to_string(1)] = instance_handle;
      }
    } else {
      instances_[f_mesh.name.data()] = instance_handle;
    }
  }

  for (auto child_index : node.children) {
    process_node(asset, child_index, world_matrix);
  }
}
