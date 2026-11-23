#pragma once

#include <common/providers.hpp>
#include <renderer/renderer.hpp>

#include "command_pool.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "image_manager.hpp"
#include "imgui_impl.hpp"
#include "material_manager.hpp"
#include "mesh_manager.hpp"
#include "pipeline_manager.hpp"
#include "renderer_types.hpp"
#include "sampler_manager.hpp"
#include "swapchain.hpp"
#include "texture_manager.hpp"

#include "instance_manager.hpp"
#include "resource_loader.hpp"

#include <chrono>
#include <memory>

class ResourceManager;

class VulkanRenderer : public Renderer {
public:
  void init();

  void begin_frame();

  void draw_frame();

  void wait();

  float get_delta_time() { return delta_time_; }

  VulkanRenderer(const VulkanRendererConfig &config);
  ~VulkanRenderer();

private:
  uint32_t frame_idx_ = 0;
  float delta_time_ = 0.0f;
  std::chrono::high_resolution_clock::time_point last_frame_ =
      std::chrono::high_resolution_clock::now();

  Camera *camera_;

  VulkanWindowProviders window_providers_;

  std::unique_ptr<Device> device_;
  VulkanContext context_;

  ResourceManager *resource_manager_;

  std::unique_ptr<Swapchain> swapchain_;
  std::unique_ptr<DescriptorAllocator> descriptor_allocator_;
  std::unique_ptr<DescriptorWriter> descriptor_writer_;
  std::unique_ptr<PipelineManager> pipeline_manager_;
  std::unique_ptr<CommandPool> graphics_pool_;
  std::unique_ptr<CommandPool> transfer_pool_;
  std::unique_ptr<MeshManager> mesh_manager_;
  std::unique_ptr<TextureManager> texture_manager_;
  std::unique_ptr<MaterialManager> material_manager_;
  std::unique_ptr<ImageManager> image_manager_;
  std::unique_ptr<SamplerManager> sampler_manager_;
  std::unique_ptr<ResourceLoader> resource_loader_;
  std::unique_ptr<InstanceManager> instance_manager_;
  std::unique_ptr<ImGuiUtil> imgui_util_;

  vk::Pipeline graphics_static_mesh_pipeline_;
  vk::PipelineLayout graphics_static_mesh_pipeline_layout_;

  std::vector<vk::raii::CommandBuffer> draw_command_buffers_;

  std::vector<vk::raii::Semaphore> render_finished_semaphores_;
  std::vector<vk::raii::Semaphore> present_complete_semaphores_;
  std::vector<vk::raii::Fence> draw_fences_;

  CommandContext command_context_;
  Descriptors descriptors_;

  void create_sync_objects();

  void record_command_buffer(uint32_t image_idx);

  FrameGlobals frame_globals_;
  vk::raii::Buffer frame_global_buffer_ = nullptr;
  vk::raii::DeviceMemory frame_global_memory_ = nullptr;
  void *frame_global_memory_mapped_;
  uint64_t frame_global_buffer_address_;
  void create_frame_global_buffer();
  void update_frame_globals();
};
