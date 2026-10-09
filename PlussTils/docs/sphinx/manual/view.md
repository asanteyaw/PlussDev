# PlussView: A Guide

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussView API Reference](../api/view.md) · [Architecture](../guides/view/architecture.md) · [Window Runtime](../guides/view/window_runtime.md)

This guide covers PlussView's drawing pipeline: a short list of commands on a
pixel frame, a renderer that paints the commands onto the frame, and a way to
save the result. This is the renderer underneath PlussView's own window --
the "built in renderer," in the codebase's own words -- that `DataFrame`,
`Atlas`, and PlussPl's plots use when their own `.show()` opens a window
instead of printing text. If that is all you want, you don't need anything on
this page: call `.show()` on the object you have, documented on its own type's
reference.

This guide exists for the lower level underneath that: building a frame,
painting it, and saving it to a file, with nothing else involved -- no
`DataFrame`, no window, no backend choice. It is real, public, and accurately
documented; whether you will ever need to call it directly, rather than
through `.show()`, depends on whether you want pixel-level control PlussView's
own callers don't expose.

This guide follows the pipeline in the order you will use it. You build a
**scene** from commands, you **render** it onto a **canvas**, you read or
**export** the canvas, and you check what happened through the render
**events**. Each stage has one job, and each example in this guide uses only
that stage's public types. Every program is complete, and it runs without a
window. Every printed line is copied from the program's real output.

## The mental model

A scene is an ordered list of commands. A command is one drawing operation, such
as clearing the frame, filling a rectangle, or setting a clip region. The scene
keeps the commands in the order you added them and does not draw anything
itself.

A renderer takes a scene and paints it onto a canvas. The canvas is a plain
frame of pixels with a fixed size, stored as RGBA bytes row by row. Rendering
applies the commands in order, so a later command covers an earlier one where
they overlap, and a clip region limits what the commands after it can change.

Rendering also reports what it did through a list of events. A frame that
renders normally reports that it started and that it finished. The events are
how you tell a successful frame from one that a command could not draw.

## Example 1: A scene, a canvas, and one fill

The first program creates a canvas eight pixels wide and four tall. It clears the
frame to a dark blue, fills a small rectangle with orange, and reads three pixels
back: one inside the rectangle, one in the background, and one just past its
right edge. Read the program as the whole pipeline in its smallest form.

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

The canvas reports eight by four. The pixel inside the rectangle has the fill
color, because the rectangle covers columns two through four and rows one and
two. The pixel at the corner is background, and the pixel just past the right
edge of the rectangle, at column five, is also background. Those three reads
show the rectangle's extent exactly, because the rectangle's right edge is
exclusive.

## Example 2: Later commands paint over earlier ones

Commands paint in the order they were added, so the scene's order is a
decision you make. The second program draws two overlapping rectangles, then
draws the same two in reverse order. The overlap takes the color of the later
command both times.

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

Where the two rectangles overlap, the later one wins. Outside the overlap, the
earlier one remains visible. Reversing the order reverses the winner, and the
scene keeps the commands exactly as you added them, so you can check its
contents before rendering. Build a scene in the order you want it to look, and
avoid relying on a renderer to sort anything for you.

## Example 3: A clip region limits what changes

A clip region restricts the commands that follow it. The third program clears the
frame to black, sets a clip covering columns two through four, and fills the
whole frame with white. Only the columns inside the clip become white. Clearing
the clip afterwards restores unrestricted drawing for any commands that follow.

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

Columns zero, five, and seven stay black because they lie outside the clip, even
though the fill asked for the whole frame. Columns two and four are white because
they lie inside it. A clip is how you confine a drawing to a region, for example
to keep a chart's lines inside its plot area. Remember that a clip applies to the
commands after it, so set it before the commands it should constrain and clear it
when you are done.

## Example 4: Exporting a frame to a file

A frame is only useful if you can keep it. The fourth program first shows that
the export format is decided by the file extension, and that an extension it
does not know produces no format at all. It then renders a small frame, writes it
as a PNG into a private temporary directory, and checks the file's first eight
bytes against the PNG signature. The program removes its directory at the end.

```cpp
#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/export.h>
#include <pluss/tils/view/window/renderer.h>
#include <pluss/tils/view/window/scene.h>
#include <pluss/tils/view/window/types.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace view = pluss::view::window;

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;

  // The export format comes from the file extension.
  std::cout << "png recognized: " << (view::detect_canvas_export_format("frame.png") == view::CanvasExportFormat::Png) << '\n';
  std::cout << "ppm recognized: " << (view::detect_canvas_export_format("frame.ppm") == view::CanvasExportFormat::Ppm) << '\n';
  std::cout << "unknown extension recognized: " << view::detect_canvas_export_format("frame.txt").has_value() << '\n';

  // Render a small frame, then write it as a PNG into a private temporary directory.
  view::PixelCanvas canvas(view::SizeI{16, 8});
  view::Scene scene;
  scene.add(view::ClearCommand{view::Color::rgba(30, 60, 90, 255)});
  scene.add(view::FillRectCommand{view::RectI{4, 2, 8, 4}, view::Color::rgba(240, 200, 40, 255)});
  auto renderer = view::make_software_renderer();
  std::vector<view::RenderEvent> events;
  renderer->render(scene, canvas, events);

  const fs::path dir = fs::temp_directory_path() / "pluss_view_export_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path target = dir / "frame.png";
  std::cout << "saved: " << view::save_canvas_png(canvas, target.string()) << '\n';

  // A PNG file starts with the same eight signature bytes in every file.
  std::ifstream in(target, std::ios::binary);
  std::vector<unsigned char> head(8);
  in.read(reinterpret_cast<char*>(head.data()), 8);
  const std::vector<unsigned char> png_signature = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  std::cout << "png signature present: " << (head == png_signature) << '\n';
  std::cout << "file is not empty: " << (fs::file_size(target) > 8) << '\n';

  fs::remove_all(dir);
}
```

Output

```text
png recognized: true
ppm recognized: true
unknown extension recognized: false
saved: true
png signature present: true
file is not empty: true
```

The extensions `.png` and `.ppm` are recognized, and `.txt` is not. The saved
file starts with the PNG signature, and it is not empty. Those checks confirm the
file format without depending on how the pixels compress, which is the part of
the output a reader is least likely to need. Choose the extension to match what
the consumer expects, because the format follows from it.

## Example 5: Scaling a scene

A chart drawn at one size often has to appear at another. The fifth program
scales a scene by a factor of two. Scaling returns a new scene and leaves the
original untouched, and it multiplies the geometry while keeping the colors.

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

The original rectangle keeps its position and size, and the scaled copy has
doubled values for all four geometric fields. The color is unchanged. Scale a
scene when the same drawing must appear at a different resolution, and keep the
original scene so that you can scale it again at another factor.

## Example 6: Events report what a frame did

Rendering reports its progress through events, and the events are the only
signal that a frame finished in a normal way. The sixth program renders a frame
and inspects the events it produced. It then clears the list and renders again,
which shows that events accumulate in the list you pass until you clear it.

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

A normal frame produces two events, the first marks the start and the last marks
the finish. The second frame, rendered after the list was cleared, produces two events
again. Check the events after each render when you care
about a frame's outcome, and clear the list between frames when you want to read
each one separately.

## Example 7: Sizes and writes that the canvas refuses

Canvases have rules about size and bounds, and they are easy to trip over
because the calls look ordinary. The seventh program covers three of them. A
default canvas has no pixels and reports empty. A write outside the canvas is
ignored, and the canvas does not grow. A zero-width resize is refused, and it
leaves the canvas empty.

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

The default canvas is empty. The out-of-bounds write leaves the four by four size
unchanged. A valid resize to six by two succeeds and takes effect. The zero-width
resize returns false, and the canvas is empty afterwards. Check the return value
of `resize`, because a refused resize does not keep the previous frame.

## Decisions

Most view work comes down to a few decisions, made in this order.

| You want | Use | Notes |
|---|---|---|
| A frame with a background | `ClearCommand` first in the scene | Later commands paint on top of it |
| One region to change, the rest untouched | `SetClipRectCommand` ... `ClearClipRectCommand` | The clip applies only to the commands between |
| The same drawing at a new resolution | `scale_scene` | Returns a new scene; the original is unchanged |
| A file on disk | `save_canvas_png` with a `.png` or `.ppm` path | The format follows from the extension |
| To know a frame succeeded | Check the render result and the events | A normal frame reports start and finish |

Two decisions carry the most risk. First, decide the paint order when you build
the scene, because the renderer applies it exactly and does not reorder anything.
Second, check the events or the render result before you trust a frame that you
intend to export. The event kinds include an unsupported-command report and an
error report, so an export does not, by itself, prove that every command was
drawn.

## Pitfalls

**Assuming a later command is drawn under an earlier one.** Example 2 shows that
the later command covers the earlier one. Put backgrounds first.

**Leaving a clip set.** Example 3 shows that a clip applies to what follows it.
Clear it before drawing anything that should not be constrained.

**Ignoring a refused resize.** Example 7 shows that a zero-width resize returns
false and empties the canvas. Check the result of `resize`.

**Writing outside the canvas and expecting it to appear.** Example 7 shows that
out-of-bounds writes are ignored. Size the canvas to the drawing you intend.

**Reusing an event list across frames.** Example 6 shows that events accumulate
until you clear the list. Clear it between frames when you read each frame's
outcome.

## Where next

- [PlussView API Reference](../api/view.md) -- the canonical view surface, one
  entry per type and operation.
- [Architecture](../guides/view/architecture.md) -- how the subsystem divides its
  responsibilities.
- [Window Runtime](../guides/view/window_runtime.md) -- how a frame reaches a
  native window when you do want one.
- [Loaders: A Guide](loaders.md) -- for reading the data a chart is drawn from.

```{toctree}
:maxdepth: 1
:hidden:

../guides/view/architecture
../guides/view/window_runtime
```
