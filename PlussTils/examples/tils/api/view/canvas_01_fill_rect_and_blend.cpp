// fill_rect paints a solid rectangle directly, without going through a Scene, and blend_pixel mixes
// a color into the existing pixel by an alpha weight, rather than replacing it outright.
#include <pluss/tils/view/window/canvas.h>
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
  view::PixelCanvas canvas(view::SizeI{4, 4});
  canvas.clear(view::Color::rgba(0, 0, 0, 255));
  canvas.fill_rect(view::RectI{1, 1, 2, 2}, view::Color::rgba(255, 0, 0, 255));
  std::cout << "inside the rect: " << int(pixel_at(canvas, 1, 1).r) << '\n';
  std::cout << "outside the rect: " << int(pixel_at(canvas, 0, 0).r) << '\n';

  canvas.blend_pixel(view::PointI{1, 1}, view::Color::rgba(0, 0, 255, 255), 128);
  const auto blended = pixel_at(canvas, 1, 1);
  std::cout << "blended red component dropped: " << (blended.r < 255) << '\n';
  std::cout << "blended blue component rose: " << (blended.b > 0) << '\n';
  return 0;
}
