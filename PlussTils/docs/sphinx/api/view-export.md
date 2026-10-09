# `pluss::view::window canvas export`

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

```cpp
#include <pluss/tils/view/window/export.h>

enum class CanvasExportFormat : unsigned char { Ppm = 0, Png = 1 };

std::optional<CanvasExportFormat> detect_canvas_export_format(std::string_view path);
bool save_canvas_ppm(const PixelCanvas& canvas, std::string_view path);
bool save_canvas_png(const PixelCanvas& canvas, std::string_view path);
bool save_canvas(const PixelCanvas& canvas, std::string_view path, CanvasExportFormat format);
bool save_canvas(const PixelCanvas& canvas, std::string_view path);
```

These functions write a `PixelCanvas` to a file, in one of two formats: PPM, a plain uncompressed format, or PNG. `detect_canvas_export_format` reads a path's extension and reports which format it names, or nothing for an extension it does not recognize. `save_canvas_ppm` and `save_canvas_png` each write their named format directly. `save_canvas` with an explicit `format` writes that format through the same named function; `save_canvas` with no format detects it from the path instead.

Every one of these functions, however you reach it, refuses to write unless the path's extension matches the format being written: a file this module writes always has a name that truthfully describes its own contents. This holds even for `save_canvas_png` called by name, and even for `save_canvas` given an explicit format; passing the format explicitly selects which writer runs, it does not let you bypass the extension check. A canvas with no pixels, or a non-positive size, is also refused.

## Parameters

| Parameter | Meaning |
|---|---|
| `canvas` | The frame to write. |
| `path` | The destination. Its extension must match the format being written. |
| `format` | The format to write, for the explicit-format overload of `save_canvas`. |

## Returns

`detect_canvas_export_format` returns the format the path names, or nothing. Every save function returns whether the write succeeded.

## Errors

None thrown. A mismatched extension, an empty canvas, or a failure to open the file for writing are all reported through the boolean return, not an exception.

## Practical Examples

### Example 1

Example 1 detects the format from three different extensions, then saves a frame as PNG and checks its signature bytes.

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

A .png path is recognized as Png, a .ppm path as Ppm, and an unrecognized extension reports nothing; the saved file begins with the standard eight-byte PNG signature.

### Example 2

Example 2 saves as PPM directly, and compares the explicit-format overload with a mismatched and a matching extension.

```cpp
// save_canvas_ppm writes the plain PPM format directly. Every save function, including the one that
// takes an explicit format, still refuses a path whose extension does not match: the file a program
// writes always has a name that truthfully describes its own contents. save_canvas with no format
// argument detects the format from the path instead of being told.
#include <pluss/tils/view/window/canvas.h>
#include <pluss/tils/view/window/export.h>
#include <pluss/tils/view/window/types.h>

#include <filesystem>
#include <iostream>

namespace view = pluss::view::window;
namespace fs = std::filesystem;

int main() {
  std::cout << std::boolalpha;
  view::PixelCanvas canvas(view::SizeI{4, 4});
  canvas.clear(view::Color::rgba(10, 20, 30, 255));

  const fs::path dir = fs::temp_directory_path() / "pluss_view_export_overloads_example";
  fs::remove_all(dir);
  fs::create_directories(dir);

  std::cout << "ppm saved: " << view::save_canvas_ppm(canvas, (dir / "frame.ppm").string()) << '\n';
  std::cout << "explicit png format, mismatched extension, refused: "
            << !view::save_canvas(canvas, (dir / "frame.dat").string(), view::CanvasExportFormat::Png) << '\n';
  std::cout << "explicit png format, matching extension, saved: "
            << view::save_canvas(canvas, (dir / "frame2.png").string(), view::CanvasExportFormat::Png) << '\n';
  std::cout << "auto-detected from extension: " << view::save_canvas(canvas, (dir / "frame.png").string()) << '\n';

  fs::remove_all(dir);
  return 0;
}
```

Output

```text
ppm saved: true
explicit png format, mismatched extension, refused: true
explicit png format, matching extension, saved: true
auto-detected from extension: true
```

The PPM save succeeds, the explicit PNG format is refused on a path that does not end in .png, and it succeeds once the extension matches.

## See Also

- [`PixelCanvas`](view-canvas.md)
