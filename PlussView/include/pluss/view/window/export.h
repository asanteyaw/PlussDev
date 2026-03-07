#pragma once

#include <optional>
#include <string_view>

#include "pluss/view/window/canvas.h"

namespace pluss::view::window {

enum class CanvasExportFormat : unsigned char {
  Ppm = 0,
  Png = 1,
};

[[nodiscard]] std::optional<CanvasExportFormat> detect_canvas_export_format(std::string_view path);
[[nodiscard]] bool save_canvas_ppm(const PixelCanvas& canvas, std::string_view path);
[[nodiscard]] bool save_canvas_png(const PixelCanvas& canvas, std::string_view path);
[[nodiscard]] bool save_canvas(const PixelCanvas& canvas,
                               std::string_view path,
                               CanvasExportFormat format);
[[nodiscard]] bool save_canvas(const PixelCanvas& canvas, std::string_view path);

} // namespace pluss::view::window
