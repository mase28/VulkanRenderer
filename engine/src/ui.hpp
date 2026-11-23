#pragma once

class Scene;

#include <functional>
#include <string>
#include <unordered_map>

class UI {
public:
  UI(Scene *scene);

  using FWidget = std::function<void()>;
  void add_window(std::string name, FWidget f);

  void draw();

private:
  Scene *scene_;
  std::unordered_map<std::string, FWidget> windows_;
};
