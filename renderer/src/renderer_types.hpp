#pragma once

#include <assets/mesh.hpp>
#include <assets/math_types.hpp>
#include <common/handles.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class CommandPool;

class ResourceManager;
class MeshManager;
class TextureManager;
class MaterialManager;
class ImageManager;
class SamplerManager;
class DescriptorAllocator;
class DescriptorWriter;

namespace Config {
  struct ResourceManagers {
    ResourceManager* resource_manager;
    MeshManager* mesh_manager;
    TextureManager* texture_manager;
    MaterialManager* material_manager;
    ImageManager* image_manager;
    SamplerManager* sampler_manager;
  };
}

struct VulkanContext {
  uint32_t max_frames_in_flight;

  vk::raii::Context context;
  vk::raii::Instance instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
  vk::raii::PhysicalDevice physical_device = nullptr;
  vk::raii::Device device = nullptr;
  vk::raii::SurfaceKHR surface = nullptr;

  // vector of queues ??? TODO
  struct Queues {
    vk::raii::Queue graphics = nullptr;
    vk::raii::Queue transfer = nullptr;
  } queues;

  struct QueueIndices {
    uint32_t graphics;
    uint32_t transfer;
  } queue_indices;
};

struct RenderTargetInfo {
  vk::Format color_format;
  vk::Extent2D extent;
};

struct CommandContext {
  CommandPool* graphics_pool;
  CommandPool* transfer_pool;
};

struct Descriptors {
  DescriptorAllocator* descriptor_allocator;
  DescriptorWriter* descriptor_writer;
};

struct FrameGlobals {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 view_proj;
  glm::vec4 camera_pos;
  glm::vec4 light_pos;
  glm::vec4 light_color;
};

struct PushConstant {
  uint64_t frame_global_addr;
  uint64_t instance_addr;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec2 tex_coord;
  glm::vec3 normal;
  glm::vec4 tangent;
};

struct VertexBuffer {
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
};

struct IndexBuffer {
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
  vk::IndexType index_type;
};

struct TransformBuffer {
  std::vector<glm::mat4> model_matrix;
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
  void* memory_mapped;
};

struct MaterialFactors {
  glm::vec4 albedo;
  float normal;
  float metallic;
  float roughness;
  float ao;
};

struct VulkanMaterial {
  vk::DescriptorSet descriptor_set;
};

struct VulkanImage {
  vk::raii::Image image = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
  vk::raii::ImageView image_view = nullptr;
};

struct VulkanSampler {
  vk::raii::Sampler sampler = nullptr;
};

struct VulkanTexture {
  Handle<Image> image_handle;
  Handle<SamplerDesc> sampler_desc;
};

struct RenderStaticMesh {
  VertexBuffer vertex_buffer;
  IndexBuffer index_buffer;
};

struct VulkanMeshInstance {
  Handle<Mesh> mesh_handle;
  TransformBuffer transform_buffer;
};
