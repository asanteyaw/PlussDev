# `pluss::view::window: PointI, SizeI, RectI, and Color`

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

These four plain structs are the vocabulary every other PlussView type is built from. `PointI` is a pixel position, `SizeI` a width and a height, and `RectI` a rectangle with both. `RectI::empty()` is true when the width or the height is not positive, which is how a canvas and the renderer decide a rectangle has nothing to draw. `Color` holds four 8-bit channels, defaulting to opaque black; `Color::rgba` is the usual way to build one, with alpha defaulting to fully opaque.

## Fields

| Type | Fields | Meaning |
|---|---|---|
| `PointI` | `x`, `y` | Pixel position; each defaults to 0. |
| `SizeI` | `width`, `height` | Pixel dimensions; each defaults to 0. |
| `RectI` | `x`, `y`, `width`, `height` | Position and extent; each defaults to 0. `empty()` is true for a non-positive extent. |
| `Color` | `r`, `g`, `b`, `a` | 8-bit channels. RGB default to 0 and alpha to 255; `rgba(...)` constructs a color. |

The types themselves do not validate dimensions; the canvas or renderer interprets them.

## Practical Examples

### Example 1

Example 1 builds a canvas from a SizeI and reads it back.

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

The canvas reports the same width and height it was built with.

### Example 2

Example 2 builds colors with Color::rgba and reads them back from rendered pixels.

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

The pixel inside the filled rectangle matches the color it was filled with, and the pixel outside keeps the clear color.

### Example 3

Example 3 builds a RectI and checks emptiness through a canvas resize.

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

A size with a zero width is treated as empty, and the canvas built from it reports empty too.

## See Also

- [`PixelCanvas`](view-canvas.md)
- [Draw commands and `Scene`](view-scene.md)
