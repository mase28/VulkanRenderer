#pragma once

#include "renderer_types.hpp"
#include <common/handles.hpp>
#include <assets/mesh.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>
#include <cstdint>

class SamplerManager {
public:
  SamplerManager(VulkanContext& context);

  void add_sampler(Handle<SamplerDesc> handle, SamplerDesc& sampler_desc);
  void remove_sampler(Handle<SamplerDesc> handle) {assert(handle.is_valid()); samplers_.erase(handle);}

  vk::Sampler get_sampler(Handle<SamplerDesc> handle) {assert(handle.is_valid()); return *samplers_.at(handle).sampler;}
  
  Handle<SamplerDesc> get_default_sampler_handle() {return default_sampler_handle_;}
private:
  VulkanContext* context_;

  std::unordered_map<Handle<SamplerDesc>, VulkanSampler> samplers_;

  Handle<SamplerDesc> default_sampler_handle_ = {.value = UINT32_MAX - 1};
};
