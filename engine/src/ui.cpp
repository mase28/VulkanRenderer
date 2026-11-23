#include "ui.hpp"

#include <scene/scene.hpp>

#include <imgui.h>

UI::UI(Scene *scene) : scene_(scene) {}

void UI::add_window(std::string name, FWidget f) {
  windows_.try_emplace(name, f);
}

void UI::draw() {
  for (auto &[name, f] : windows_) {
    ImGui::Begin(name.c_str());
    f();
    ImGui::End();
  }
}
