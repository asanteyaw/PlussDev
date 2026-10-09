// A scene can hold any of the fourteen draw command types. This renders a stroked rectangle, a
// line, and a piece of text, to show the pattern holds beyond fills.
#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;
  view::PixelCanvas canvas(view::SizeI{20, 20});
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(255, 255, 255, 255)});
  scene.add(view::StrokeRectCommand{view::RectI{2, 2, 10, 10}, 1, view::Color::rgba(0, 0, 0, 255)});
  scene.add(view::LineCommand{view::PointI{0, 0}, view::PointI{19, 19}, 1, view::Color::rgba(255, 0, 0, 255)});
  scene.add(view::TextCommand{view::PointI{1, 15}, "hi", 10, view::Color::rgba(0, 128, 0, 255)});

  auto renderer = view::make_software_renderer();
  std::vector<view::RenderEvent> events;
  const bool ok = renderer->render(scene, canvas, events);
  std::cout << "rendered: " << ok << '\n';
  std::cout << "commands held: " << scene.commands().size() << '\n';
  return 0;
}
