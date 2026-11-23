#pragma once

#include <assets/mesh.hpp>
#include <common/providers.hpp>

#include <cstdint>
#include <memory>
#include <vector>

class Scene;
class ResourceManager;
class Camera;

struct VulkanRendererConfig {
  uint32_t max_frames_in_flight, width, height;
  std::vector<const char *> extensions;
  VulkanWindowProviders window_providers;
  Camera *camera;
  Scene *scene;
  ResourceManager *resource_manager;
};

class Renderer {
public:
  virtual ~Renderer() = default;

  virtual void init() = 0;

  virtual void begin_frame() = 0;

  virtual void draw_frame() = 0;

  virtual void wait() = 0;

  virtual float get_delta_time() = 0;
};

std::unique_ptr<Renderer> create_renderer(const VulkanRendererConfig &config);
