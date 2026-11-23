//
// Created by mase28 on 5/19/26.
//
#pragma once

#include "renderer_types.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

class Swapchain;

class ImGuiUtil {
public:
  ImGuiUtil(const VulkanContext &context, const CommandContext &command_context,
            Swapchain *swapchain);
  ~ImGuiUtil();

  void new_frame();

  void render(vk::raii::CommandBuffer &command_buffer);

private:
  const VulkanContext *context_;
  const CommandContext *command_context_;
  Swapchain *swapchain_;

  vk::raii::DescriptorPool imgui_pool_ = nullptr;
};
