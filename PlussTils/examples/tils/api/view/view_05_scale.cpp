#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>
#include <variant>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;
  // Scaling a scene multiplies its geometry. It returns a new scene and leaves the original alone.
  view::Scene scene;
  scene.add(view::FillRectCommand{view::RectI{2, 3, 4, 5}, view::Color::rgba(1, 2, 3, 255)});
  const view::Scene doubled = view::scale_scene(scene, 2.0);

  const auto& original = std::get<view::FillRectCommand>(scene.commands().front());
  const auto& scaled = std::get<view::FillRectCommand>(doubled.commands().front());
  std::cout << "original rect: " << original.rect.x << ' ' << original.rect.y << ' '
            << original.rect.width << ' ' << original.rect.height << '\n';
  std::cout << "scaled rect: " << scaled.rect.x << ' ' << scaled.rect.y << ' '
            << scaled.rect.width << ' ' << scaled.rect.height << '\n';
  std::cout << "colors kept: " << (scaled.color.r == 1 && scaled.color.g == 2 && scaled.color.b == 3) << '\n';
}
