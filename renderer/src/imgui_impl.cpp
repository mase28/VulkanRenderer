#include "imgui_impl.hpp"

#include "renderer_types.hpp"
#include "swapchain.hpp"

#include "imgui.h"
#include <backends/imgui_impl_vulkan.h>

#include <array>

ImGuiUtil::ImGuiUtil(const VulkanContext &context,
                     const CommandContext &command_context,
                     Swapchain *swapchain)
    : context_(&context), command_context_(&command_context),
      swapchain_(swapchain) {
  vk::DescriptorPoolSize sampled_image{
      .type = vk::DescriptorType::eSampledImage,
      .descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE};
  vk::DescriptorPoolSize sampler{
      .type = vk::DescriptorType::eSampler,
      .descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE};
  std::array<vk::DescriptorPoolSize, 2> pool_size{sampled_image, sampler};

  vk::DescriptorPoolCreateInfo pool_info{
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE *
                 IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE,
      .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
      .pPoolSizes = pool_size.data()};
  imgui_pool_ = vk::raii::DescriptorPool{context_->device, pool_info};

  RenderTargetInfo render_target = swapchain_->get_render_target();
  vk::PipelineRenderingCreateInfoKHR pipeline_info{
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &render_target.color_format};

  ImGui_ImplVulkan_InitInfo init_info{
      .ApiVersion = VK_API_VERSION_1_3,
      .Instance = *context_->instance,
      .PhysicalDevice = *context_->physical_device,
      .Device = *context_->device,
      .QueueFamily = context_->queue_indices.graphics,
      .Queue = *context_->queues.graphics,
      .DescriptorPool = *imgui_pool_,
      .MinImageCount = context_->max_frames_in_flight,
      .ImageCount = context_->max_frames_in_flight,
      .PipelineInfoMain = {.MSAASamples =
                               VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
                           .PipelineRenderingCreateInfo = pipeline_info},
      .UseDynamicRendering = true};
  auto load_func = [](const char *func_name, void *user_data) {
    return vkGetInstanceProcAddr(static_cast<VkInstance>(user_data), func_name);
  };
  ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, load_func,
                                 static_cast<VkInstance>(*context_->instance));

  ImGui_ImplVulkan_Init(&init_info);
}

ImGuiUtil::~ImGuiUtil() {
  ImGui_ImplVulkan_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiUtil::new_frame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
}

void ImGuiUtil::render(vk::raii::CommandBuffer &command_buffer) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);
}
