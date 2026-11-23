#pragma once

#include <common/providers.hpp>
#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <iostream>

class Swapchain {
public:
  Swapchain(VulkanContext& context, VulkanWindowProviders* window_providers);
  //~Swapchain();
  RenderTargetInfo get_render_target();
  uint32_t image_count() {return swapchain_images_.size();}
  vk::Image get_image(uint32_t image_idx) {return swapchain_images_[image_idx];}
  vk::ImageView get_image_view(uint32_t image_idx) {return *swapchain_image_views_[image_idx];}
  vk::Extent2D get_extent() {return swapchain_extent_;}
  vk::SampleCountFlagBits get_num_samples() {return msaa_samples_;}
  vk::Image get_color_image() {return *color_image_;}
  vk::ImageView get_color_image_view() {return color_image_view_;}
  vk::Image get_depth_image() {return *depth_image_;}
  vk::ImageView get_depth_image_view() {return *depth_image_view_;}
  vk::Format get_depth_format() {return depth_format_;}
  void recreate_swapchain();
  const vk::SwapchainKHR& get_handle() {return *swapchain_;}
  vk::ResultValue<uint32_t> acquire_next_image(uint64_t timeout, vk::Semaphore semaphore, vk::Fence fence) {return swapchain_.acquireNextImage(UINT64_MAX, semaphore, fence);}
private:
  VulkanContext* context_;
  VulkanWindowProviders* window_providers_;

  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::SurfaceFormatKHR surface_format_;
  vk::Format swapchain_image_format_ = vk::Format::eB8G8R8A8Srgb;
  vk::Extent2D swapchain_extent_;
  vk::raii::SwapchainKHR swapchain_ = nullptr;
  std::vector<vk::Image> swapchain_images_;
  std::vector<vk::raii::ImageView> swapchain_image_views_;

  vk::raii::Image depth_image_ = nullptr;
  vk::raii::DeviceMemory depth_image_memory_ = nullptr;
  vk::raii::ImageView depth_image_view_ = nullptr;
  vk::Format depth_format_;

  vk::SampleCountFlagBits msaa_samples_ = vk::SampleCountFlagBits::e1;
  vk::SampleCountFlagBits get_max_sample_count();

  vk::raii::Image color_image_ = nullptr;
  vk::raii::DeviceMemory color_image_memory_ = nullptr;
  vk::raii::ImageView color_image_view_ = nullptr;

  bool create_swapchain();
  vk::raii::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect, uint32_t mip_levels);
  bool create_image_views();
  bool create_depth_resources();

  bool create_color_resources();

  void cleanup_swapchain();
};