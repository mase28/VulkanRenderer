#include "mesh_manager.hpp"
#include "utils.hpp"
#include "command_pool.hpp"

#include <iostream>

MeshManager::MeshManager(VulkanContext& context, CommandContext& command_context) : context_(&context), command_context_(&command_context) {

}

void MeshManager::load_static_mesh(Handle<Mesh> handle, Mesh& mesh) {
  RenderStaticMesh render_mesh;
  render_mesh.vertex_buffer = create_vertex_buffer(mesh);
  render_mesh.index_buffer = create_index_buffer(mesh.indices);

  static_meshes_[handle] = std::move(render_mesh);
  //std::cout << "Mesh loaded with ID: " << mesh.handle.value << std::endl;
}

void MeshManager::unload_static_mesh(Handle<Mesh> handle) {
  assert(handle.is_valid());
  static_meshes_.erase(handle);
  //std::cout << "Mesh unloaded with ID: " << handle.value << std::endl;
}

void MeshManager::copy_buffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size, vk::PipelineStageFlags2 dst_stage_mask, vk::AccessFlags2 dst_access_mask) {
  // Acquire and release to transfer ownership of buffer from transfer queue to graphics queue

  // Copy and release memory barrier from transfer queue
  vk::raii::CommandBuffer transfer_command = command_context_->transfer_pool->allocate_single_buffer();

  vk::CommandBufferBeginInfo begin_info {
    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
  };
  transfer_command.begin(begin_info);

  transfer_command.copyBuffer(src, dst, vk::BufferCopy{0, 0, size});

  vk::BufferMemoryBarrier2 release_barrier {
    .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eNone,
    .dstAccessMask = vk::AccessFlagBits2::eNone,
    .srcQueueFamilyIndex = context_->queue_indices.transfer,
    .dstQueueFamilyIndex = context_->queue_indices.graphics,
    .buffer = dst,
    .offset = 0,
    .size = size
  };
  vk::DependencyInfo transfer_dependency_info {
    .bufferMemoryBarrierCount = 1,
    .pBufferMemoryBarriers = &release_barrier
  };
  transfer_command.pipelineBarrier2(transfer_dependency_info);

  transfer_command.end();

  vk::SubmitInfo transfer_submit {
    .commandBufferCount = 1,
    .pCommandBuffers = &*transfer_command
  };

  // Acquire for graphics queue
  vk::raii::CommandBuffer graphics_command = command_context_->graphics_pool->allocate_single_buffer();
  graphics_command.begin(begin_info);

  vk::BufferMemoryBarrier2 acquire_barrier {
    .srcStageMask = vk::PipelineStageFlagBits2::eNone,
    .srcAccessMask = vk::AccessFlagBits2::eNone,
    .dstStageMask = dst_stage_mask,
    .dstAccessMask = dst_access_mask,
    .srcQueueFamilyIndex = context_->queue_indices.transfer,
    .dstQueueFamilyIndex = context_->queue_indices.graphics,
    .buffer = dst,
    .offset = 0,
    .size = size
  };
  vk::DependencyInfo graphics_dependency_info {
    .bufferMemoryBarrierCount = 1,
    .pBufferMemoryBarriers = &acquire_barrier
  };
  graphics_command.pipelineBarrier2(graphics_dependency_info);

  graphics_command.end();

  vk::SubmitInfo graphics_submit {
    .commandBufferCount = 1,
    .pCommandBuffers = &*graphics_command
  };

  // Now we do the submission and use fence for graphics queue to wait on transfer
  auto fence = context_->device.createFence({});

  context_->queues.transfer.submit(transfer_submit, fence);
  auto transfer_fence_result = context_->device.waitForFences(*fence, true, UINT64_MAX);
  if (transfer_fence_result != vk::Result::eSuccess) {
    throw std::runtime_error("Unable to wait for transfer fence");
  }
  context_->device.resetFences(*fence);

  context_->queues.graphics.submit(graphics_submit, fence);
  auto graphics_fence_result = context_->device.waitForFences(*fence, true, UINT64_MAX);
  if (graphics_fence_result != vk::Result::eSuccess) {
    throw std::runtime_error("Unable to wait for graphics fence");
  }

  //std::cout << "Buffer has been loaded into GPU memory\n";
}

VertexBuffer MeshManager::create_vertex_buffer(const Mesh& mesh) {
  std::vector<Vertex> vertices(mesh.vertex_pos.size());
  for (size_t i = 0; i < mesh.vertex_pos.size(); i++) {
    vertices[i] = Vertex {mesh.vertex_pos[i], mesh.vertex_tex_coord[i], mesh.vertex_normal[i], mesh.vertex_tangent[i]};
  }
  vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

  vk::raii::Buffer staging_buffer = nullptr;
  vk::raii::DeviceMemory staging_buffer_memory = nullptr;
  utils::create_buffer(context_, buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

  void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
  memcpy(data_staging, vertices.data(), buffer_size);
  staging_buffer_memory.unmapMemory();

  VertexBuffer vertex_buffer;
  utils::create_buffer(context_, buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, vertex_buffer.buffer, vertex_buffer.memory);

  copy_buffer(staging_buffer, vertex_buffer.buffer, buffer_size, vk::PipelineStageFlagBits2::eVertexAttributeInput, vk::AccessFlagBits2::eVertexAttributeRead);

  //std::cout << "Vertex buffer created\n";
  return vertex_buffer;
}

IndexBuffer MeshManager::create_index_buffer(const std::vector<uint32_t>& indices) {
  vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

  vk::raii::Buffer staging_buffer = nullptr;
  vk::raii::DeviceMemory staging_buffer_memory = nullptr;
  utils::create_buffer(context_, buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, staging_buffer, staging_buffer_memory);

  void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
  memcpy(data_staging, indices.data(), buffer_size);
  staging_buffer_memory.unmapMemory();

  IndexBuffer index_buffer;
  index_buffer.index_type = vk::IndexType::eUint32;

  utils::create_buffer(context_, buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, index_buffer.buffer, index_buffer.memory);

  copy_buffer(staging_buffer, index_buffer.buffer, buffer_size, vk::PipelineStageFlagBits2::eIndexInput, vk::AccessFlagBits2::eIndexRead);

  //std::cout << "Index buffer created\n";
  return index_buffer;
}
