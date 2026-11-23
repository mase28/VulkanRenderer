#include "vulkan_renderer.hpp"
#include "utils.hpp"
#include <assets/math_types.hpp>
#include <assets/mesh.hpp>
#include <camera/camera.hpp>
#include <resource_manager/resource_manager.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>

#include <vulkan/vulkan_raii.hpp>

void VulkanRenderer::init() { std::cout << "Init called\n"; }

void VulkanRenderer::begin_frame() { imgui_util_->new_frame(); }

void VulkanRenderer::draw_frame() {

  auto fence_result = context_.device.waitForFences(*draw_fences_[frame_idx_],
                                                    vk::True, UINT64_MAX);
  if (fence_result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to wait for fence\n");
  }

  if (window_providers_.size_changed()) {
    std::cout << "Frame buffer resized\n";
    swapchain_->recreate_swapchain();
    return;
  }

  auto now = std::chrono::high_resolution_clock::now();
  delta_time_ = std::chrono::duration<float>(now - last_frame_).count();
  last_frame_ = now;

  uint32_t image_idx;
  try {
    auto [result, index] = swapchain_->acquire_next_image(
        UINT64_MAX, *present_complete_semaphores_[frame_idx_], nullptr);
    image_idx = index;
  } catch (vk::OutOfDateKHRError) {
    std::cout << "out of date khr error\n";
    swapchain_->recreate_swapchain();
    return;
  }

  context_.device.resetFences(*draw_fences_[frame_idx_]);

  draw_command_buffers_[frame_idx_].reset();

  record_command_buffer(image_idx);

  update_frame_globals();

  vk::PipelineStageFlags wait_destination_stage_mask{
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  const vk::SubmitInfo submit_info{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*present_complete_semaphores_[frame_idx_],
      .pWaitDstStageMask = &wait_destination_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers = &*draw_command_buffers_[frame_idx_],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*render_finished_semaphores_[image_idx]};
  context_.queues.graphics.submit(submit_info, *draw_fences_[frame_idx_]);

  const vk::PresentInfoKHR present_info_khr{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*render_finished_semaphores_[image_idx],
      .swapchainCount = 1,
      .pSwapchains = &swapchain_->get_handle(),
      .pImageIndices = &image_idx};

  try {
    vk::Result result = context_.queues.graphics.presentKHR(present_info_khr);
    if (result == vk::Result::eSuboptimalKHR) {
      swapchain_->recreate_swapchain();
    }
  } catch (vk::OutOfDateKHRError) {
    swapchain_->recreate_swapchain();
  }

  frame_idx_ = (frame_idx_ + 1) % context_.max_frames_in_flight;
}

void VulkanRenderer::wait() { std::cout << "Wait called\n"; }

VulkanRenderer::VulkanRenderer(const VulkanRendererConfig &config)
    : window_providers_(config.window_providers), camera_(config.camera),
      resource_manager_(config.resource_manager) {

  context_.max_frames_in_flight = config.max_frames_in_flight;
  device_ =
      std::make_unique<Device>(config.extensions, &window_providers_, true);
  device_->build_context(context_);

  swapchain_ = std::make_unique<Swapchain>(context_, &window_providers_);
  descriptor_allocator_ = std::make_unique<DescriptorAllocator>(context_);
  descriptor_writer_ = std::make_unique<DescriptorWriter>();

  descriptors_ = {.descriptor_allocator = descriptor_allocator_.get(),
                  .descriptor_writer = descriptor_writer_.get()};

  graphics_pool_ = std::make_unique<CommandPool>(
      context_, context_.queue_indices.graphics,
      vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  transfer_pool_ =
      std::make_unique<CommandPool>(context_, context_.queue_indices.transfer,
                                    vk::CommandPoolCreateFlagBits::eTransient);
  command_context_.graphics_pool = graphics_pool_.get();
  command_context_.transfer_pool = transfer_pool_.get();

  draw_command_buffers_ =
      graphics_pool_->allocate_buffers(context_.max_frames_in_flight);

  create_sync_objects();

  image_manager_ = std::make_unique<ImageManager>(context_, command_context_);
  sampler_manager_ = std::make_unique<SamplerManager>(context_);
  mesh_manager_ = std::make_unique<MeshManager>(context_, command_context_);
  texture_manager_ = std::make_unique<TextureManager>(image_manager_.get(),
                                                      sampler_manager_.get());
  material_manager_ = std::make_unique<MaterialManager>(
      context_, descriptors_, texture_manager_.get(), image_manager_.get(),
      sampler_manager_.get());

  Config::ResourceManagers resource_loader_managers{
      .resource_manager = config.resource_manager,
      .mesh_manager = mesh_manager_.get(),
      .texture_manager = texture_manager_.get(),
      .material_manager = material_manager_.get(),
      .image_manager = image_manager_.get(),
      .sampler_manager = sampler_manager_.get()};
  resource_loader_ = std::make_unique<ResourceLoader>(
      context_, command_context_, resource_loader_managers);
  instance_manager_ = std::make_unique<InstanceManager>(
      context_, descriptors_, config.scene, resource_loader_managers);

  pipeline_manager_ = std::make_unique<PipelineManager>(context_);

  // change pipeline creation to dynamic
  PipelineCreateInfo create_info{
      .pipeline_type = PipelineType::StaticMesh,
      .shader_file_name = "pbr_slang.spv",
      .descriptor_layouts = {descriptors_.descriptor_allocator->get_layout(
          material_manager_->get_default_layout_handle())},
      .render_target = swapchain_->get_render_target(),
      .depth_format = swapchain_->get_depth_format(),
      .num_samples = swapchain_->get_num_samples(),
      .push_constants = {{.stageFlags = vk::ShaderStageFlagBits::eVertex |
                                        vk::ShaderStageFlagBits::eFragment,
                          .offset = 0,
                          .size = sizeof(PushConstant)}}};
  graphics_static_mesh_pipeline_ =
      pipeline_manager_->create_graphics_pipeline(create_info);
  graphics_static_mesh_pipeline_layout_ =
      pipeline_manager_->get_pipeline_layout(PipelineType::StaticMesh);

  create_frame_global_buffer();

  imgui_util_ =
      std::make_unique<ImGuiUtil>(context_, command_context_, swapchain_.get());
}

VulkanRenderer::~VulkanRenderer() {
  context_.device.waitIdle();
  std::cout << "Object destroyed\n";
}

void VulkanRenderer::create_sync_objects() {
  for (size_t i = 0; i < swapchain_->image_count(); i++) {
    render_finished_semaphores_.emplace_back(context_.device,
                                             vk::SemaphoreCreateInfo{});
  }
  if (render_finished_semaphores_.empty()) {
    throw std::runtime_error("Unable to create render finished semaphores");
  }
  for (size_t i = 0; i < context_.max_frames_in_flight; i++) {
    present_complete_semaphores_.emplace_back(context_.device,
                                              vk::SemaphoreCreateInfo{});
    draw_fences_.emplace_back(
        context_.device,
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
  }
  if (present_complete_semaphores_.empty()) {
    throw std::runtime_error("Unable to create present complete semaphores");
  } else if (draw_fences_.empty()) {
    throw std::runtime_error("Unable to create draw fences");
  }
  std::cout << "Created sync objects\n";
}

void VulkanRenderer::record_command_buffer(uint32_t image_idx) {
  auto &command_buffer = draw_command_buffers_[frame_idx_];
  command_buffer.begin({});

  utils::transition_image_layout(
      command_buffer, swapchain_->get_image(image_idx),
      vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor, 1);
  utils::transition_image_layout(
      command_buffer, swapchain_->get_color_image(),
      vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor, 1);
  utils::transition_image_layout(
      command_buffer, swapchain_->get_depth_image(),
      vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
      vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      vk::PipelineStageFlagBits2::eEarlyFragmentTests |
          vk::PipelineStageFlagBits2::eLateFragmentTests,
      vk::PipelineStageFlagBits2::eEarlyFragmentTests |
          vk::PipelineStageFlagBits2::eLateFragmentTests,
      vk::ImageAspectFlagBits::eDepth, 1);

  vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.0f, 0);
  vk::RenderingAttachmentInfo color_attachment_info{
      .imageView = swapchain_->get_color_image_view(),
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .resolveMode = vk::ResolveModeFlagBits::eAverage,
      .resolveImageView = swapchain_->get_image_view(image_idx),
      .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clear_color};
  vk::RenderingAttachmentInfo depth_attachment_info{
      .imageView = swapchain_->get_depth_image_view(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = clear_depth};
  vk::RenderingInfo rendering_info{
      .renderArea = {.offset{0, 0}, .extent = swapchain_->get_extent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info};

  command_buffer.beginRendering(rendering_info);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                              graphics_static_mesh_pipeline_);

  command_buffer.setViewport(
      0, vk::Viewport{0, 0, static_cast<float>(swapchain_->get_extent().width),
                      static_cast<float>(swapchain_->get_extent().height), 0.0f,
                      1.0f});
  command_buffer.setScissor(
      0, vk::Rect2D(vk::Offset2D{0, 0}, swapchain_->get_extent()));

  // TODO get all instances from scene and load their vertex and index buffers
  // from asset loader
  command_buffer.pushConstants(graphics_static_mesh_pipeline_layout_,
                               vk::ShaderStageFlagBits::eVertex |
                                   vk::ShaderStageFlagBits::eFragment,
                               offsetof(PushConstant, frame_global_addr),
                               sizeof(uint64_t), &frame_global_buffer_address_);
  const std::unordered_map<Handle<MeshInstance>, VulkanMeshInstance>
      &mesh_instances = instance_manager_->get_instances();
  for (auto &[instance_id, mesh_instance] : mesh_instances) {
    auto &vk_mesh = mesh_manager_->get_static_mesh(mesh_instance.mesh_handle);
    command_buffer.bindVertexBuffers(0, *vk_mesh.vertex_buffer.buffer, {0});
    command_buffer.bindIndexBuffer(vk_mesh.index_buffer.buffer, 0,
                                   vk_mesh.index_buffer.index_type);

    // Get buffer address then offset it by the current frame to get the
    // instance's current frame transform
    uint64_t buffer_addr = context_.device.getBufferAddress(
        {.buffer = mesh_instance.transform_buffer.buffer});
    buffer_addr = buffer_addr + (sizeof(glm::mat4) * frame_idx_);
    command_buffer.pushConstants(
        graphics_static_mesh_pipeline_layout_,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        offsetof(PushConstant, instance_addr), sizeof(uint64_t), &buffer_addr);

    auto &mesh = resource_manager_->get_mesh(mesh_instance.mesh_handle);
    for (auto &primitive : mesh.primitives) {
      command_buffer.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          graphics_static_mesh_pipeline_layout_, 0,
          material_manager_->get_material(primitive.material_handle)
              .descriptor_set,
          nullptr);
      command_buffer.drawIndexed(primitive.index_count, 1,
                                 primitive.first_index, 0, 0);
    }
  }

  command_buffer.endRendering();

  imgui_util_->render(command_buffer);

  utils::transition_image_layout(
      command_buffer, swapchain_->get_image(image_idx),
      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite, {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe,
      vk::ImageAspectFlagBits::eColor, 1);

  command_buffer.end();
}

void VulkanRenderer::create_frame_global_buffer() {
  utils::create_buffer(&context_, sizeof(frame_globals_),
                       vk::BufferUsageFlagBits::eUniformBuffer |
                           vk::BufferUsageFlagBits::eShaderDeviceAddress,
                       vk::MemoryPropertyFlagBits::eHostCoherent |
                           vk::MemoryPropertyFlagBits::eHostVisible,
                       frame_global_buffer_, frame_global_memory_);
  frame_global_memory_mapped_ =
      frame_global_memory_.mapMemory(0, sizeof(frame_globals_));

  vk::BufferDeviceAddressInfo address_info{.buffer = frame_global_buffer_};
  frame_global_buffer_address_ = context_.device.getBufferAddress(address_info);
}

void VulkanRenderer::update_frame_globals() {
  frame_globals_.view = camera_->get_view();
  frame_globals_.proj =
      glm::perspective(glm::radians(45.0f),
                       static_cast<float>(swapchain_->get_extent().width) /
                           static_cast<float>(swapchain_->get_extent().height),
                       0.1f, 100.0f);
  frame_globals_.proj[1][1] *= -1;
  frame_globals_.view_proj = frame_globals_.proj * frame_globals_.view;
  frame_globals_.camera_pos = glm::vec4(camera_->get_position(), 0);

  // Hard Coded for now
  frame_globals_.light_pos = glm::vec4{0, 0, 10, 0};
  frame_globals_.light_color = glm::vec4(1.0f);

  memcpy(frame_global_memory_mapped_, &frame_globals_, sizeof(frame_globals_));
}
