#include <filesystem>
#include <string_view>

#include "pluss/view/window.h"

namespace {

bool require(const bool condition) {
  return condition;
}

bool file_has_bytes(const std::filesystem::path& path) {
  std::error_code ec{};
  return std::filesystem::exists(path, ec) && !ec &&
         std::filesystem::is_regular_file(path, ec) && !ec &&
         std::filesystem::file_size(path, ec) > 0U && !ec;
}

} // namespace

int main() {
  using namespace pluss::view::window;

  const std::filesystem::path out_dir = "PlussView/examples/window";
  const std::filesystem::path png_path = out_dir / "export_smoke_ok.png";
  const std::filesystem::path ppm_path = out_dir / "export_smoke_ok.ppm";
  const std::filesystem::path bad_ext_path = out_dir / "export_smoke_bad.dat";

  PixelCanvas canvas(SizeI{16, 12});
  canvas.clear(Color::rgba(12, 34, 56, 255));
  canvas.set_pixel(PointI{0, 0}, Color::rgba(255, 0, 0, 255));
  canvas.set_pixel(PointI{15, 11}, Color::rgba(0, 255, 0, 255));

  if (!require(detect_canvas_export_format("plot.png") == CanvasExportFormat::Png)) {
    return 1;
  }
  if (!require(detect_canvas_export_format("plot.ppm") == CanvasExportFormat::Ppm)) {
    return 2;
  }
  if (!require(!detect_canvas_export_format("plot.pdf").has_value())) {
    return 3;
  }

  if (!require(save_canvas_png(canvas, png_path.string()))) {
    return 4;
  }
  if (!require(save_canvas_ppm(canvas, ppm_path.string()))) {
    return 5;
  }
  if (!require(save_canvas(canvas, png_path.string()))) {
    return 6;
  }
  if (!require(save_canvas(canvas, ppm_path.string()))) {
    return 7;
  }

  if (!require(!save_canvas_png(canvas, ppm_path.string()))) {
    return 8;
  }
  if (!require(!save_canvas_ppm(canvas, png_path.string()))) {
    return 9;
  }
  if (!require(!save_canvas(canvas, bad_ext_path.string()))) {
    return 10;
  }

  if (!require(file_has_bytes(png_path))) {
    return 11;
  }
  if (!require(file_has_bytes(ppm_path))) {
    return 12;
  }

  return 0;
}
