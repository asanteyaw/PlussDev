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
