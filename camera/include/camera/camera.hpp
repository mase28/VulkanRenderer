#pragma once

#include <assets/math_types.hpp>

#include <glm/glm.hpp>

class Camera {
public:
  Camera(glm::vec3 position);

  void move_direction(glm::vec3 direction, float delta_time);
  void look_at(glm::vec2 mouse_location);

  const glm::mat4& get_view();
  const glm::vec3& get_position() {return pos_;}
private:
  float speed_ = 10.0f;
  float sensitivity = 0.1f;

  bool first_call = true;
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;

  float yaw = 90.0f;
  float pitch = 0.0f;

  glm::vec3 pos_ = {0.0f, 0.0f, 0.0f};
  glm::vec3 forward_ = {0.0f, 0.0f, 1.0f};
  glm::vec3 up_ = {0.0f, 1.0f, 0.0f};

  glm::mat4 view_;
};