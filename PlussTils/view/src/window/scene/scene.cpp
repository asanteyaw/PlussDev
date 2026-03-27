#include "pluss/tils/view/window/scene.h"

#include <utility>

namespace pluss::view::window {

void Scene::clear_commands() {
  commands_.clear();
}

void Scene::add(DrawCommand command) {
  commands_.push_back(std::move(command));
}

const std::vector<DrawCommand>& Scene::commands() const noexcept {
  return commands_;
}

} // namespace pluss::view::window
