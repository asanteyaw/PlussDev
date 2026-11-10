# `pluss::view::window::Scene and the draw commands`

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

```cpp
using DrawCommand = std::variant<
    ClearCommand, FillRectCommand, FillRoundedRectCommand, FillLinearGradientCommand,
    StrokeRectCommand, StrokeRoundedRectCommand, LineCommand, PolylineCommand,
    FillPolygonCommand, MarkerCloudCommand, SetClipRectCommand, ClearClipRectCommand,
    TextCommand, TextCaretCommand>;



Scene scale_scene(const Scene& scene, double scale);
```

**Header:** `<pluss/tils/view/window/scene.h>`

### `Scene` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `add` | Method | `void add(DrawCommand command)` |
| `clear_commands` | Method | `void clear_commands()` |
| `commands` | Method | `const std::vector<DrawCommand>& commands() const noexcept` |

A `Scene` retains an ordered list of draw commands; it does not paint anything itself. `add` appends one command, accepting any of the fourteen alternatives of `DrawCommand` through its implicit conversion. `commands()` reads the list back, in the order the commands were added, and `clear_commands` empties it. A renderer applies the list in that same order, so a later command paints over an earlier one wherever they overlap; nothing about a `Scene` reorders or merges commands.

The fourteen command types are plain data: `ClearCommand` fills the whole frame; `FillRectCommand`, `FillRoundedRectCommand`, and `FillLinearGradientCommand` fill a rectangle, solid, rounded, or with a gradient; `StrokeRectCommand` and `StrokeRoundedRectCommand` outline a rectangle; `LineCommand` and `PolylineCommand` draw straight segments; `FillPolygonCommand` fills an arbitrary polygon; `MarkerCloudCommand` draws a dot at each of a list of points; `SetClipRectCommand` and `ClearClipRectCommand` set and clear a clip that limits every command between them; `TextCommand` draws a line of text, and `TextCaretCommand` draws a text input's caret. Each one is a simple aggregate; build it with its fields and add it.

`scale_scene` returns a new `Scene` with every command's geometry multiplied by `scale`; the scene it was given is unchanged. Colors and other non-geometric fields are kept as they are.

## Parameters

| Parameter | Meaning |
|---|---|
| `command` | Any of the fourteen `DrawCommand` alternatives. |
| `scene` | The scene to scale, for `scale_scene`. |
| `scale` | The factor every geometric field is multiplied by. |

## Returns

`add` and `clear_commands` return nothing. `commands()` returns the retained list. `scale_scene` returns a new, independent `Scene`.

## Errors

None. A `Scene` accepts any command and never validates its fields; the renderer decides what an invalid command means.

## Practical Examples

### Example 1

Example 1 adds commands and renders them, and reads the commands list back.

```cpp
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
```

Output

```text
overlap takes the later fill: true
earlier fill survives outside overlap: true
reversed order takes the red fill: true
commands retained: 2
```

A later fill paints over an earlier one where they overlap, reversing the order reverses which one wins, and commands() reports how many were added.

### Example 2

Example 2 sets and clears a clip rectangle around a fill.

```cpp
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
```

Output

```text
column 0 stays black: true
column 2 is filled: true
column 4 is filled: true
column 5 stays black: true
column 7 stays black: true
```

Only the columns inside the clip change; the rest of the frame keeps its earlier color.

### Example 3

Example 3 clears a scene's command list.

```cpp
// clear_commands empties a scene's command list, the same way a default-constructed scene starts.
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <iostream>

namespace view = pluss::view::window;

int main() {
  view::Scene scene;
  scene.add(view::ClearCommand{});
  scene.add(view::FillRectCommand{});
  std::cout << "commands before clearing: " << scene.commands().size() << '\n';
  scene.clear_commands();
  std::cout << "commands after clearing: " << scene.commands().size() << '\n';
  return 0;
}
```

Output

```text
commands before clearing: 2
commands after clearing: 0
```

The command count drops to zero after clear_commands, the same as a scene that never had anything added.

### Example 4

Example 4 adds a stroked rectangle, a line, and text, beyond the fill and clip commands.

```cpp
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
```

Output

```text
rendered: true
commands held: 4
```

All three render without being refused, and the scene reports the number of commands it holds.

### Example 5

Example 5 scales a scene and compares the original and the scaled command.

```cpp
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
```

Output

```text
original rect: 2 3 4 5
scaled rect: 4 6 8 10
colors kept: true
```

The scaled rectangle's position and size are each multiplied by the scale factor, the color is unchanged, and the original scene is untouched.

## See Also

- [`PixelCanvas`](view-canvas.md)
- [`SoftwareRenderer`](view-renderer.md) -- applies a Scene's commands to a canvas.
