#pragma once

#include <common/handles.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <cstdint>
#include <functional>

struct Image {
  std::vector<uint8_t> pixels;
  uint32_t width;
  uint32_t height;
};

namespace SamplerConfig {
  enum class MagFilter {
    NEAREST,
    LINEAR
  };
  enum class MinFilter {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
  };
  enum class WrapS {
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT,
    REPEAT
  };
  enum class WrapT {
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT,
    REPEAT
  };
}

struct SamplerDesc {
  SamplerConfig::MagFilter mag_filter;
  SamplerConfig::MinFilter min_filter;
  SamplerConfig::WrapS wrap_s;
  SamplerConfig::WrapT wrap_t;

  bool operator==(const SamplerDesc& other) const {
    return (mag_filter == other.mag_filter) && (min_filter == other.min_filter) && (wrap_s == other.wrap_s) && (wrap_t == other.wrap_t);
  }
};

namespace std {
  template <>
  struct hash<SamplerDesc> {
    size_t operator()(const SamplerDesc& desc) const {
      return hash<uint8_t>()(static_cast<uint8_t>(desc.mag_filter)) ^ hash<uint8_t>()(static_cast<uint8_t>(desc.min_filter)) << 1 ^ hash<uint8_t>()(static_cast<uint8_t>(desc.wrap_s)) << 2 ^ hash<uint8_t>()(static_cast<uint8_t>(desc.wrap_t)) << 3;
    }
  };
};

struct Texture {
  Handle<Image> image_handle;
  bool has_sampler = false;
  Handle<SamplerDesc> sampler_handle;
};

struct Material {
  Handle<Texture> albedo_texture_handle;
  glm::vec4 albedo_texture_factor;
  Handle<Texture> normal_texture_handle;
  float normal_texture_scale;
  Handle<Texture> metallic_roughness_texture_handle;
  float metallic_factor;
  float roughness_factor;
  Handle<Texture> ao;
  float ao_strength;
};

struct Primitive {
  uint32_t index_count;
  uint32_t first_index;
  Handle<Material> material_handle;
};

struct Mesh {
  std::vector<Primitive> primitives;
  
  std::vector<glm::vec3> vertex_pos;
  std::vector<glm::vec2> vertex_tex_coord;
  std::vector<glm::vec3> vertex_normal;
  std::vector<glm::vec4> vertex_tangent;

  std::vector<uint32_t> indices;
};

struct MeshInstance {
  Handle<Mesh> mesh_handle;
  glm::mat4 model_matrix;
};
