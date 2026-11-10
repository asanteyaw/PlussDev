# `pluss::view::window::PixelCanvas`

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

**Header:** `<pluss/tils/view/window/canvas.h>`

### `PixelCanvas` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `PixelCanvas` | Method | `PixelCanvas() = default` |
| `PixelCanvas` | Method | `explicit PixelCanvas(SizeI size)` |
| `resize` | Method | `bool resize(SizeI size)` |
| `clear` | Method | `void clear(Color color) noexcept` |
| `fill_rect` | Method | `void fill_rect(RectI rect, Color color) noexcept` |
| `set_pixel` | Method | `void set_pixel(PointI pixel, Color color) noexcept` |
| `blend_pixel` | Method | `void blend_pixel(PointI pixel, Color color, std::uint8_t alpha) noexcept` |
| `size` | Method | `SizeI size() const noexcept` |
| `empty` | Method | `bool empty() const noexcept` |
| `bytes` | Method | `std::span<const std::uint8_t> bytes() const noexcept` |

`PixelCanvas` owns a plain RGBA frame buffer. A default-constructed canvas has no pixels and reports empty; building one with a `SizeI`, or calling `resize` later, allocates a frame of that size. `resize` returns whether it succeeded: a non-positive width or height is refused, and the canvas is left empty rather than keeping its previous frame. `clear`, `fill_rect`, `set_pixel`, and `blend_pixel` paint directly onto the canvas, without going through a `Scene`; a coordinate or rectangle that falls outside the canvas is ignored rather than refused. `blend_pixel` mixes a color into the existing pixel by an alpha weight, instead of replacing it outright. `bytes()` exposes the raw RGBA buffer, four bytes per pixel, row by row.

## Parameters

| Parameter | Meaning |
|---|---|
| `size` | The frame's width and height, for the constructor and `resize`. |
| `color` | The color to paint. |
| `rect` | The rectangle to fill. |
| `pixel` | The pixel to set or blend. |
| `alpha` | The blend weight, 0 to 255, for `blend_pixel`. |

## Returns

`resize` returns whether the new size was accepted. The paint methods return nothing. `size`, `empty`, and `bytes` read the canvas's current state.

## Errors

None of these throw. An invalid `resize` is refused by its return value, and an out-of-bounds write is silently ignored.

## Practical Examples

### Example 1

Example 1 builds a canvas, renders onto it through a Scene, and reads pixels back.

```cpp
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
```

Output

```text
canvas: 8 x 4
rendered: true
inside: 200 100 50 255
outside: 10 20 30 255
just past the right edge: 10 20 30 255
```

The filled rectangle's pixels match the fill color, and pixels outside it keep the clear color.

### Example 2

Example 2 paints directly with fill_rect and blend_pixel, without a Scene.

```cpp
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
```

Output

```text
inside the rect: 255
outside the rect: 0
blended red component dropped: 1
blended blue component rose: 1
```

fill_rect paints the rectangle's pixels outright, and blend_pixel mixes a second color into one of them rather than replacing it.

### Example 3

Example 3 writes outside the canvas and resizes it, once validly and once not.

```cpp
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
```

Output

```text
default canvas empty: true
size after out-of-bounds write: 4 x 4
valid resize succeeds: true
size after valid resize: 6 x 2
zero width resize succeeds: false
canvas empty after refused resize: true
```

A default canvas is empty, an out-of-bounds write changes nothing, a valid resize succeeds and takes effect, and a zero-width resize is refused and leaves the canvas empty.

## See Also

- [Draw commands and `Scene`](view-scene.md)
- [`SoftwareRenderer`](view-renderer.md) -- paints a Scene onto a canvas.
- [Export](view-export.md)
