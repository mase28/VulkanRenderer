#include "sampler_manager.hpp"

SamplerManager::SamplerManager(VulkanContext& context) : context_(&context) {
  vk::PhysicalDeviceProperties properties = context_->physical_device.getProperties();
  vk::SamplerCreateInfo default_create_info {
    .magFilter = vk::Filter::eLinear,
    .minFilter = vk::Filter::eLinear,
    .mipmapMode = vk::SamplerMipmapMode::eLinear,
    .addressModeU = vk::SamplerAddressMode::eRepeat,
    .addressModeV = vk::SamplerAddressMode::eRepeat,
    .addressModeW = vk::SamplerAddressMode::eRepeat,
    .mipLodBias = 0.0f,
    .anisotropyEnable = vk::True,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .compareEnable = vk::False,
    .compareOp = vk::CompareOp::eAlways,
    .minLod = 0.0f,
    .maxLod = vk::LodClampNone
  };
  VulkanSampler default_sampler;
  default_sampler.sampler = vk::raii::Sampler(context_->device, default_create_info);
  samplers_.try_emplace(default_sampler_handle_, std::move(default_sampler));
}

void SamplerManager::add_sampler(Handle<SamplerDesc> handle, SamplerDesc& sampler_desc) {
  vk::PhysicalDeviceProperties properties = context_->physical_device.getProperties();
  vk::SamplerCreateInfo create_info {
    .mipmapMode = vk::SamplerMipmapMode::eLinear,
    .mipLodBias = 0.0f,
    .anisotropyEnable = vk::True,
    .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    .compareEnable = vk::False,
    .compareOp = vk::CompareOp::eAlways,
    .minLod = 0.0f,
    .maxLod = vk::LodClampNone
  };
  
  switch (sampler_desc.mag_filter) {
    case SamplerConfig::MagFilter::LINEAR:
      create_info.magFilter = vk::Filter::eLinear;
      break;
    case SamplerConfig::MagFilter::NEAREST:
      create_info.magFilter = vk::Filter::eNearest;
      break;
  }

  switch (sampler_desc.min_filter) {
    case SamplerConfig::MinFilter::LINEAR:
      create_info.minFilter = vk::Filter::eLinear;
      create_info.maxLod = 0.0f;
      break;
    case SamplerConfig::MinFilter::NEAREST:
      create_info.minFilter = vk::Filter::eNearest;
      create_info.maxLod = 0.0f;
      break;
    case SamplerConfig::MinFilter::NEAREST_MIPMAP_LINEAR:
      create_info.minFilter = vk::Filter::eNearest;
      create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
      break;
    case SamplerConfig::MinFilter::NEAREST_MIPMAP_NEAREST:
      create_info.minFilter = vk::Filter::eNearest;
      create_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
      break;
    case SamplerConfig::MinFilter::LINEAR_MIPMAP_LINEAR:
      create_info.minFilter = vk::Filter::eLinear;
      create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
      break;
    case SamplerConfig::MinFilter::LINEAR_MIPMAP_NEAREST:
      create_info.minFilter = vk::Filter::eLinear;
      create_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
      break;
  }

  switch (sampler_desc.wrap_s) {
    case SamplerConfig::WrapS::CLAMP_TO_EDGE:
      create_info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
      break;
    case SamplerConfig::WrapS::MIRRORED_REPEAT:
      create_info.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
      break;
    case SamplerConfig::WrapS::REPEAT:
      create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
      break;
  }

  switch (sampler_desc.wrap_t) {
    case SamplerConfig::WrapT::CLAMP_TO_EDGE:
      create_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
      break;
    case SamplerConfig::WrapT::MIRRORED_REPEAT:
      create_info.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
      break;
    case SamplerConfig::WrapT::REPEAT:
      create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
      break;
  }

  // gltf does not have option for w so set it equal to x
  create_info.addressModeW = create_info.addressModeU;

  VulkanSampler sampler;
  sampler.sampler = vk::raii::Sampler(context_->device, create_info);

  samplers_.try_emplace(handle, std::move(sampler));
}
