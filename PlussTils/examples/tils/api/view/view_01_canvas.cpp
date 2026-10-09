#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <cstddef>
#include <iostream>

namespace view = pluss::view::window;

namespace {

// Reads one pixel from the canvas's RGBA bytes.
view::Color pixel_at(const view::PixelCanvas& canvas, int x, int y) {
  const auto bytes = canvas.bytes();
  const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.size().width) +
                             static_cast<std::size_t>(x)) * 4;
  return view::Color{bytes[index], bytes[index + 1], bytes[index + 2], bytes[index + 3]};
}

void print_color(const char* label, view::Color c) {
  std::cout << label << ": " << int(c.r) << ' ' << int(c.g) << ' ' << int(c.b) << ' ' << int(c.a) << '\n';
}

} // namespace

int main() {
  // A canvas is a plain RGBA frame. Its size is fixed until you resize it.
  view::PixelCanvas canvas(view::SizeI{8, 4});
  std::cout << "canvas: " << canvas.size().width << " x " << canvas.size().height << '\n';

  // A scene retains commands in the order they were added. Clear the frame, then fill a rectangle.
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(10, 20, 30, 255)});
  scene.add(view::FillRectCommand{view::RectI{2, 1, 3, 2}, view::Color::rgba(200, 100, 50, 255)});

  auto renderer = view::make_software_renderer();
  std::vector<view::RenderEvent> events;
  const bool ok = renderer->render(scene, canvas, events);
  std::cout << "rendered: " << std::boolalpha << ok << '\n';

  // Read pixels inside and outside the rectangle.
  print_color("inside", pixel_at(canvas, 3, 2));
  print_color("outside", pixel_at(canvas, 0, 0));
  print_color("just past the right edge", pixel_at(canvas, 5, 2));
}
