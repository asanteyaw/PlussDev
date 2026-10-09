#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;

  // A default canvas has no pixels at all, so it reports empty.
  view::PixelCanvas none;
  std::cout << "default canvas empty: " << none.empty() << '\n';

  // Writing outside the canvas is ignored. It does not grow the frame.
  view::PixelCanvas canvas(view::SizeI{4, 4});
  canvas.set_pixel(view::PointI{100, 100}, view::Color::rgba(255, 0, 0, 255));
  std::cout << "size after out-of-bounds write: " << canvas.size().width << " x " << canvas.size().height << '\n';

  // Resizing to a valid size succeeds and replaces the frame.
  std::cout << "valid resize succeeds: " << canvas.resize(view::SizeI{6, 2}) << '\n';
  std::cout << "size after valid resize: " << canvas.size().width << " x " << canvas.size().height << '\n';

  // A zero width is not a valid frame. The resize is refused and the canvas is left empty.
  std::cout << "zero width resize succeeds: " << canvas.resize(view::SizeI{0, 2}) << '\n';
  std::cout << "canvas empty after refused resize: " << canvas.empty() << '\n';
}
