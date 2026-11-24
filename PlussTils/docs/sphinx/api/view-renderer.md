# `pluss::view::window::SoftwareRenderer and RenderEvent`

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

```cpp
std::unique_ptr<IRenderer> make_software_renderer();
```

**Header:** `<pluss/tils/view/window/renderer.h>`

### `IRenderer` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `render` | Method | `virtual bool render(const Scene& scene, PixelCanvas& canvas, std::vector<RenderEvent>& events) = 0` |

**Type:** `SoftwareRenderer`

`IRenderer` is the interface a renderer implements: `render` applies a `Scene`'s commands to a `PixelCanvas`, in order, and appends one `RenderEvent` per notable thing that happened during the frame to the vector you pass in. `SoftwareRenderer` is the one implementation PlussView ships, a pure-software rasterizer with no GPU or platform dependency; `make_software_renderer` builds one behind the `IRenderer` interface, which is the usual way to get one.

A normal frame appends a `FrameStarted` event first and a `FrameFinished` event last. `UnsupportedCommand` and `Error` are the other two kinds, for a command the renderer could not apply or a failure during the frame; `render`'s own boolean return is the overall success, independent of what ended up in the events. The events vector is not cleared by `render`, so repeated calls accumulate events from every frame into the one vector, unless you clear it yourself between frames.

## Events

| Kind | Meaning |
|---|---|
| `FrameStarted` | Rendering began. |
| `FrameFinished` | Rendering finished. |
| `UnsupportedCommand` | A command could not be applied. |
| `Error` | A frame operation failed. |

Each `RenderEvent` contains a `kind` and a `note` string.

## Parameters

| Parameter | Meaning |
|---|---|
| `scene` | The commands to apply. |
| `canvas` | The frame to paint onto. |
| `events` | The vector `render` appends to. Not cleared automatically. |

## Returns

`render` returns whether the frame succeeded overall. `make_software_renderer` returns an owning pointer to a `SoftwareRenderer`, through the `IRenderer` interface.

## Errors

None thrown. A problem during a frame is reported through an `Error` or `UnsupportedCommand` event and the boolean return, not an exception.

## Practical Examples

### Example 1

Example 1 builds a renderer with make_software_renderer and renders a scene onto a canvas.

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

render returns true, and the canvas's pixels match what the scene's commands painted.

### Example 2

Example 2 reads the events of a normal frame.

```cpp
#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>
#include <vector>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;
  auto renderer = view::make_software_renderer();

  // A normal frame reports that it started and that it finished, in that order.
  view::PixelCanvas canvas(view::SizeI{4, 4});
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(0, 0, 0, 255)});
  std::vector<view::RenderEvent> events;
  const bool ok = renderer->render(scene, canvas, events);
  std::cout << "rendered ok: " << ok << '\n';
  std::cout << "event count: " << events.size() << '\n';
  std::cout << "first event started: " << (events.front().kind == view::RenderEventKind::FrameStarted) << '\n';
  std::cout << "last event finished: " << (events.back().kind == view::RenderEventKind::FrameFinished) << '\n';

  // Events accumulate in the vector you pass. Clear it between frames to read each frame separately.
  events.clear();
  renderer->render(scene, canvas, events);
  std::cout << "second frame event count: " << events.size() << '\n';
}
```

Output

```text
rendered ok: true
event count: 2
first event started: true
last event finished: true
second frame event count: 2
```

The first event is FrameStarted and the last is FrameFinished, and a second render call without clearing the vector adds its events to what is already there.

## See Also

- [Draw commands and `Scene`](view-scene.md)
- [`PixelCanvas`](view-canvas.md)
