#pragma once

#include <optional>
#include <string_view>

#include "pluss/tils/view/window/canvas.h"

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

// Writes the canvas as a one-page PDF whose page is the canvas's size in points, with the image
// run-length encoded. Needs no libraries, so every platform can save a PDF; macOS uses the
// system PDF writer instead.
[[nodiscard]] bool save_canvas_pdf(const PixelCanvas& canvas, std::string_view path);

} // namespace pluss::view::window
