#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <cstddef>
#include <iostream>

namespace view = pluss::view::window;

namespace {

view::Color pixel_at(const view::PixelCanvas& canvas, int x, int y) {
  const auto bytes = canvas.bytes();
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.size().width) +
                             static_cast<std::size_t>(x)) * 4;
  return view::Color{bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3]};
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  view::PixelCanvas canvas(view::SizeI{6, 6});
  auto renderer = view::make_software_renderer();
  std::vector<view::RenderEvent> events;

  // Commands paint in the order they were added. A later fill covers an earlier one where they overlap.
  view::Scene scene;
  scene.add(view::FillRectCommand{view::RectI{0, 0, 4, 4}, view::Color::rgba(255, 0, 0, 255)});
  scene.add(view::FillRectCommand{view::RectI{2, 2, 4, 4}, view::Color::rgba(0, 0, 255, 255)});
  renderer->render(scene, canvas, events);
  std::cout << "overlap takes the later fill: " << (pixel_at(canvas, 3, 3).b == 255) << '\n';
  std::cout << "earlier fill survives outside overlap: " << (pixel_at(canvas, 0, 0).r == 255) << '\n';

  // Reversing the order reverses the winner. The same two commands, added the other way round.
  view::Scene reversed;
  reversed.add(view::FillRectCommand{view::RectI{2, 2, 4, 4}, view::Color::rgba(0, 0, 255, 255)});
  reversed.add(view::FillRectCommand{view::RectI{0, 0, 4, 4}, view::Color::rgba(255, 0, 0, 255)});
  renderer->render(reversed, canvas, events);
  std::cout << "reversed order takes the red fill: " << (pixel_at(canvas, 3, 3).r == 255) << '\n';

  // The commands are retained, so you can inspect what the scene holds before rendering.
  std::cout << "commands retained: " << scene.commands().size() << '\n';
}
