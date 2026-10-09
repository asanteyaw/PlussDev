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
  view::PixelCanvas canvas(view::SizeI{8, 4});
  auto renderer = view::make_software_renderer();
  std::vector<view::RenderEvent> events;

  // A clip rectangle limits everything painted after it. The fill covers the whole frame,
  // but only the clipped columns change.
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(0, 0, 0, 255)});
  scene.add(view::SetClipRectCommand{view::RectI{2, 0, 3, 4}});
  scene.add(view::FillRectCommand{view::RectI{0, 0, 8, 4}, view::Color::rgba(255, 255, 255, 255)});
  scene.add(view::ClearClipRectCommand{});
  renderer->render(scene, canvas, events);

  std::cout << "column 0 stays black: " << (pixel_at(canvas, 0, 1).r == 0) << '\n';
  std::cout << "column 2 is filled: " << (pixel_at(canvas, 2, 1).r == 255) << '\n';
  std::cout << "column 4 is filled: " << (pixel_at(canvas, 4, 1).r == 255) << '\n';
  std::cout << "column 5 stays black: " << (pixel_at(canvas, 5, 1).r == 0) << '\n';
  std::cout << "column 7 stays black: " << (pixel_at(canvas, 7, 1).r == 0) << '\n';
}
