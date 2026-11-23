#include "swapchain.hpp"
#include "utils.hpp"

#include <iostream>

vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
  for (const auto available_format : available_formats) {
    if (available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return available_format;
    }
  }
  return available_formats[0];
}



vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
  for (const auto& available_present_mode : available_present_modes) {
    if (available_present_mode == vk::PresentModeKHR::eMailbox) {
      return available_present_mode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, int width, int height) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
    std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

Swapchain::Swapchain(VulkanContext& context, VulkanWindowProviders* window_providers) : context_(&context), window_providers_(window_providers) {
  msaa_samples_ = get_max_sample_count();
  if (!create_swapchain()) {
    throw std::runtime_error("Unable to create swapchain");
  }
  if (!create_image_views()) {
    throw std::runtime_error("Unable to create swapchain image views");
  }
  if (!create_color_resources()) {
    throw std::runtime_error("Unable to create color resources");
  }
  if (!create_depth_resources()) {
    throw std::runtime_error("Unable to create depth resources");
  }
}

RenderTargetInfo Swapchain::get_render_target() {
  return {swapchain_image_format_, swapchain_extent_};
}

vk::SampleCountFlagBits Swapchain::get_max_sample_count() {
  vk::PhysicalDeviceProperties properties = context_->physical_device.getProperties();

  vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
  if (counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
  if (counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
  if (counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
  if (counts & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
  if (counts & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
  if (counts & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;

  return vk::SampleCountFlagBits::e1;
}

bool Swapchain::create_swapchain() {
  auto surface_capabilities = context_->physical_device.getSurfaceCapabilitiesKHR(context_->surface);

  surface_format_ = choose_swap_surface_format(context_->physical_device.getSurfaceFormatsKHR(context_->surface));

  int width, height;
  window_providers_->window_size_provider(width, height);
  std::cout << "Width: " << width << ", Height: " << height << std::endl;
  swapchain_extent_ = choose_swap_extent(surface_capabilities, width, height);

  auto min_image_count = std::max(3u, surface_capabilities.minImageCount);
  if (surface_capabilities.maxImageCount > 0 && min_image_count > surface_capabilities.maxImageCount) {
    min_image_count = surface_capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR swapchain_create_info {
    .flags = vk::SwapchainCreateFlagsKHR(),
    .surface = context_->surface,
    .minImageCount = min_image_count,
    .imageFormat = surface_format_.format,
    .imageColorSpace = surface_format_.colorSpace,
    .imageExtent = swapchain_extent_,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .imageSharingMode = vk::SharingMode::eExclusive,
    .preTransform = surface_capabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = choose_swap_present_mode(context_->physical_device.getSurfacePresentModesKHR(*context_->surface)),
    .clipped = true,
    .oldSwapchain = swapchain_
  };
  swapchain_ = vk::raii::SwapchainKHR(context_->device, swapchain_create_info);
  if (swapchain_ == nullptr) {
    std::cout << "Unable to create swapchain\n";
    return false;
  }
  std::cout << "Created Swapchain\n";

  swapchain_images_ = swapchain_.getImages();
  
  return true;
}

vk::raii::ImageView Swapchain::create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspect, uint32_t mip_levels) {
  vk::ImageViewCreateInfo create_info {
    .image = image,
    .viewType = vk::ImageViewType::e2D,
    .format = format,
    .subresourceRange = {aspect, 0, mip_levels, 0, 1}
  };
  return vk::raii::ImageView(context_->device, create_info);
}

bool Swapchain::create_image_views() {
  swapchain_image_views_.clear();
  for (uint32_t i = 0; i < swapchain_images_.size(); i++) {
    swapchain_image_views_.push_back(create_image_view(swapchain_images_[i], swapchain_image_format_, vk::ImageAspectFlagBits::eColor, 1));
  }
  if (swapchain_image_views_.empty()) {
    std::cout << "Unable to create swapchain image views\n";
    return false;
  }
  return true;
}

bool Swapchain::create_color_resources() {
  vk::Format color_format = swapchain_image_format_;
  utils::create_image(context_, swapchain_extent_.width, swapchain_extent_.height, 1, msaa_samples_, color_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, color_image_, color_image_memory_);
  color_image_view_ = create_image_view(*color_image_, color_format, vk::ImageAspectFlagBits::eColor, 1);
  if (color_image_ == nullptr || color_image_memory_ == nullptr || color_image_view_ == nullptr) {
    return false;
  }
  return true;
}

bool Swapchain::create_depth_resources() {
  depth_format_ = utils::find_supported_format(context_, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
  utils::create_image(context_, swapchain_extent_.width, swapchain_extent_.height, 1, msaa_samples_, depth_format_, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depth_image_, depth_image_memory_);
  depth_image_view_ = create_image_view(*depth_image_, depth_format_, vk::ImageAspectFlagBits::eDepth, 1);
  if (depth_image_ == nullptr || depth_image_memory_ == nullptr || depth_image_view_ == nullptr) {
    return false;
  }
  return true;
}

void Swapchain::cleanup_swapchain() {
  swapchain_images_.clear();
  swapchain_image_views_.clear();
}

void Swapchain::recreate_swapchain() {
  std::cout << "Recreating swapchain\n";
  int width, height;
  window_providers_->window_size_provider(width, height);
  while (width == 0 || height == 0) {
    window_providers_->window_size_provider(width, height);
    window_providers_->wait_events();
  }
  std::cout << "New width: " << width << ", new height: " << height << std::endl;
  context_->device.waitIdle();

  cleanup_swapchain();
  create_swapchain();
  create_image_views();
  create_color_resources();
  create_depth_resources();
}