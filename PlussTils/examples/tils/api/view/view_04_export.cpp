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
