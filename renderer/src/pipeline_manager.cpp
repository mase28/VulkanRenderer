#include "pipeline_manager.hpp"

#include <assets/math_types.hpp>

#include <fstream>

std::vector<char> read_shader(const std::string& file_name) {
  std::string updated_file_name = "PlutoEnginev2/renderer/shaders/" + file_name;
  std::ifstream file(updated_file_name, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(std::format("Unable to open file: {}", updated_file_name));
  }

  std::vector<char> buffer(file.tellg());

  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

  file.close();
  return buffer;
}

vk::VertexInputBindingDescription get_binding_description(const Vertex& v) {
  return {0, sizeof(v), vk::VertexInputRate::eVertex};
}

std::array<vk::VertexInputAttributeDescription, 4> get_attribute_descriptions(const Vertex& v) {
  return {vk::VertexInputAttributeDescription {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)},
          vk::VertexInputAttributeDescription {1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)},
          vk::VertexInputAttributeDescription {2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
          vk::VertexInputAttributeDescription {3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)}};
}

[[nodiscard]] vk::raii::ShaderModule PipelineManager::create_shader_module(const std::vector<char>& code) const {
  vk::ShaderModuleCreateInfo create_info {
    .codeSize = code.size() * sizeof(char),
    .pCode = reinterpret_cast<const uint32_t*>(code.data())
  };
  vk::raii::ShaderModule shader_module {context_->device, create_info};
  return shader_module;
}

PipelineManager::PipelineManager(VulkanContext& context) : context_(&context) {

}

vk::Pipeline PipelineManager::create_graphics_pipeline(PipelineCreateInfo& create_info) {
  vk::raii::ShaderModule shader_module = create_shader_module(read_shader(create_info.shader_file_name));

  vk::PipelineShaderStageCreateInfo vert_shader_info {
    .stage = vk::ShaderStageFlagBits::eVertex,
    .module = shader_module,
    .pName = "vert_main"
  };
  vk::PipelineShaderStageCreateInfo frag_shader_info {
    .stage = vk::ShaderStageFlagBits::eFragment,
    .module = shader_module,
    .pName = "frag_main"
  };
  vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_shader_info, frag_shader_info};

  auto binding_description = get_binding_description(Vertex{});
  auto attribute_description = get_attribute_descriptions(Vertex{});
  vk::PipelineVertexInputStateCreateInfo vertex_input_info {
   .vertexBindingDescriptionCount = 1,
   .pVertexBindingDescriptions = &binding_description,
   .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size()),
   .pVertexAttributeDescriptions = attribute_description.data()
  };

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_info {
    .topology = vk::PrimitiveTopology::eTriangleList
  };

  vk::PipelineViewportStateCreateInfo viewport_state_info {
    .viewportCount = 1,
    .scissorCount = 1,
  };

  std::vector<vk::DynamicState> dynamic_states {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo dynamic_state = {
    .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
    .pDynamicStates = dynamic_states.data()
  };

  vk::PipelineRasterizationStateCreateInfo rasterizer_info {
    .depthClampEnable = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = vk::False,
    .depthBiasSlopeFactor = 1.0f,
    .lineWidth = 1.0f
  };

  vk::PipelineMultisampleStateCreateInfo multisampling_info {
    .rasterizationSamples = create_info.num_samples,
    .sampleShadingEnable = vk::False
  };

  vk::PipelineColorBlendAttachmentState color_blend_attachment_info {
    .blendEnable = vk::False,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };
  vk::PipelineColorBlendStateCreateInfo color_blending_info {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_info
  };

  vk::PipelineLayoutCreateInfo layout_info {
    .setLayoutCount = static_cast<uint32_t>(create_info.descriptor_layouts.size()),
    .pSetLayouts = create_info.descriptor_layouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(create_info.push_constants.size()),
    .pPushConstantRanges = create_info.push_constants.data()
  };
  pipeline_layouts_.try_emplace(create_info.pipeline_type, context_->device, layout_info);

  vk::PipelineDepthStencilStateCreateInfo depth_stencil_info {
    .depthTestEnable = vk::True,
    .depthWriteEnable = vk::True,
    .depthCompareOp = vk::CompareOp::eLess,
    .depthBoundsTestEnable = vk::False,
    .stencilTestEnable = vk::False
  };
  
  vk::PipelineRenderingCreateInfo pipeline_rendering_info {
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &create_info.render_target.color_format,
    .depthAttachmentFormat = create_info.depth_format
  };

  vk::GraphicsPipelineCreateInfo pipeline_info {
    .pNext = &pipeline_rendering_info,
    .stageCount = 2,
    .pStages = shader_stages,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly_info,
    .pViewportState = &viewport_state_info,
    .pRasterizationState = &rasterizer_info,
    .pMultisampleState = &multisampling_info,
    .pDepthStencilState = &depth_stencil_info,
    .pColorBlendState = &color_blending_info,
    .pDynamicState = &dynamic_state,
    .layout = pipeline_layouts_.at(create_info.pipeline_type),
    .renderPass = nullptr
  };
  pipelines_.try_emplace(create_info.pipeline_type, context_->device, nullptr, pipeline_info);
  if (pipelines_.at(create_info.pipeline_type) == nullptr) {
    throw std::runtime_error("Unable to create pipeline");
  }

  return *pipelines_.at(create_info.pipeline_type); 
}
