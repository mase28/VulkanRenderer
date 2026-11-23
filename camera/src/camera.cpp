#include <camera/camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>
#include <format>

Camera::Camera(glm::vec3 pos) {
  pos_ = {pos.x, pos.y, pos.z};
}

void Camera::move_direction(glm::vec3 direction, float delta_time) {
  glm::vec3 right = glm::normalize(glm::cross(forward_, up_));
  pos_ += forward_ * direction.z * speed_ * delta_time;
  pos_ += right * direction.x * speed_ * delta_time;
  pos_ += up_ * direction.y * speed_ * delta_time;
}

void Camera::look_at(glm::vec2 mouse_location) {
  // if (std::isnan(mouse_location.x) || std::isnan(mouse_location.y)) {
  //   return;
  // }
  // if (first_call) {
  //   mouse_x = mouse_location.x;
  //   mouse_y = mouse_location.y;
  //   first_call = false;
  //   std::cout << "first call\n";
  //   return;
  // }
  // float delta_x = glm::clamp(mouse_x - mouse_location.x, -50.0f, 50.0f);
  // float delta_y = glm::clamp(mouse_y - mouse_location.y, -50.0f, 50.0f);

  float delta_x = glm::clamp( mouse_location.x, -50.0f, 50.0f);
  float delta_y = glm::clamp(mouse_location.y, -50.0f, 50.0f);

  yaw += delta_x * sensitivity;
  pitch += delta_y * sensitivity;
  pitch = glm::clamp(pitch, -85.0f, 85.0f);
  
  glm::vec3 forward;
  forward.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  forward.y = -1.0f * std::sin(glm::radians(pitch));
  forward.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  
  forward_ = glm::normalize(forward);

  //mouse_x = mouse_location.x;
  //mouse_y = mouse_location.y;
  //forward_ = {0, 0, 1};
}

const glm::mat4& Camera::get_view() {
  // std::cout << std::format("pos: {},{},{}", pos_.x, pos_.y, pos_.z) << std::endl;
  // std::cout << std::format("forward: {},{},{}", forward_.x, forward_.y, forward_.z) << std::endl;
  // std::cout << std::format("up: {},{},{}", up_.x, up_.y, up_.z) << std::endl;
  view_ = glm::lookAt(pos_, pos_+forward_, up_);
  return view_;
}