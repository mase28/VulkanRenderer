#pragma once

#include "renderer_types.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

enum class PipelineType {
  StaticMesh
};

struct PipelineCreateInfo {
  PipelineType pipeline_type;
  const char* shader_file_name;
  std::vector<vk::DescriptorSetLayout> descriptor_layouts;
  RenderTargetInfo render_target;
  vk::Format depth_format;
  vk::SampleCountFlagBits num_samples;
  std::vector<vk::PushConstantRange> push_constants;
};

class PipelineManager {
public:
  PipelineManager(VulkanContext& context);

  vk::Pipeline create_graphics_pipeline(PipelineCreateInfo& create_info);
  vk::PipelineLayout get_pipeline_layout(PipelineType pipeline_type) {return *pipeline_layouts_.at(pipeline_type);}
private:
  VulkanContext* context_;

  std::unordered_map<PipelineType,vk::raii::PipelineLayout> pipeline_layouts_;
  std::unordered_map<PipelineType,vk::raii::Pipeline> pipelines_;

  vk::raii::ShaderModule create_shader_module(const std::vector<char>& code) const;
};