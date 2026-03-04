#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plot/modules/backend/pdf/primitives.h"
#include "plot/modules/backend/raster_surface_2d.h"
#include "plot/modules/backend/render_3d_shared.h"
#include "plot/modules/backend/svg/primitives.h"
#include "plot/modules/detail/show_helpers.h"
#include "plot/modules/figure3d.h"

namespace pluss::plotism {

enum class View4DLayout : std::uint8_t {
  Single = 0,
  Grid = 1,
};

class View4D;

struct View4DPanel {
  Figure3D scene{};
  std::string title{};
};

namespace detail {
void render_4d_view(RenderSurface2D& surface, const View4D& view, const RenderTarget& target);
}

class View4D {
public:
  View4D() = default;

  View4D(std::string title, Style style, View4DLayout layout, std::vector<View4DPanel> panels)
      : title_(std::move(title)), style_(std::move(style)), layout_(layout), panels_(std::move(panels)) {}

  View4D& with_title(std::string title) {
    title_ = std::move(title);
    return *this;
  }

  View4D& style(const Style& style) {
    style_ = style;
    return *this;
  }

  View4D& layout(View4DLayout layout) {
    layout_ = layout;
    return *this;
  }

  const std::string& title() const noexcept { return title_; }
  const Style& view_style() const noexcept { return style_; }
  View4DLayout layout() const noexcept { return layout_; }
  const std::vector<View4DPanel>& panels() const noexcept { return panels_; }
  std::size_t panel_count() const noexcept { return panels_.size(); }

  void show(std::ostream& os) const {
    os << "View4D(panels=" << panels_.size();
    if (!title_.empty()) {
      os << ", title=\"" << title_ << "\"";
    }
    os << ")\n";
    for (const auto& panel : panels_) {
      os << "  - " << (panel.title.empty() ? std::string("panel") : panel.title) << "\n";
    }
    os << "\n";
  }

  void show() const {
#if PLUSSPL_ENABLE_INTERACTIVE_SHOW
    try {
      if (show_window(detail::popup_plot_viewer_available(), OutputFormat::Svg)) {
        return;
      }
    } catch (...) {
      // Fallback to textual preview.
    }
#endif
    show(std::cout);
  }

  bool show_window(bool block = false,
                   OutputFormat format = OutputFormat::Svg,
                   std::int32_t width = 1400,
                   std::int32_t height = 960) const {
    const std::string path = detail::make_temp_plot_path("plusspl_4d", detail::output_extension(format));
    save_(path, width, height, format);
    return detail::open_plot_viewer(title_, path, block, width, height);
  }

  bool show_popup(OutputFormat format = OutputFormat::Svg,
                  std::int32_t width = 1400,
                  std::int32_t height = 960) const {
    return show_window(true, format, width, height);
  }

  void save_svg(std::string_view filepath, std::int32_t width = 1400, std::int32_t height = 960) const {
    save_(filepath, width, height, OutputFormat::Svg);
  }

  void save_png(std::string_view filepath, std::int32_t width = 1400, std::int32_t height = 960) const {
    save_(filepath, width, height, OutputFormat::Png);
  }

  void save_pdf(std::string_view filepath, std::int32_t width = 1400, std::int32_t height = 960) const {
    save_(filepath, width, height, OutputFormat::Pdf);
  }

private:
  void save_(std::string_view filepath, std::int32_t width, std::int32_t height, OutputFormat format) const {
    const RenderTarget target{.path = filepath, .width = width, .height = height, .format = format};
    switch (format) {
      case OutputFormat::Svg: {
        detail::SvgRenderSurface2D surface;
        detail::render_4d_view(surface, *this, target);
        std::string svg;
        svg.reserve(2048);
        svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"";
        svg += std::to_string(width);
        svg += "\" height=\"";
        svg += std::to_string(height);
        svg += "\" viewBox=\"0 0 ";
        svg += std::to_string(width);
        svg += " ";
        svg += std::to_string(height);
        svg += "\">\n";
        svg += surface.finish();
        svg += "</svg>\n";
        detail::write_text_file(filepath, svg);
        return;
      }
      case OutputFormat::Png: {
        detail::RasterSurface2D surface(width, height, 2);
        detail::render_4d_view(surface, *this, target);
        detail::write_text_file(filepath, surface.finish());
        return;
      }
      case OutputFormat::Pdf: {
        detail::PdfRenderSurface2D surface(width, height);
        detail::render_4d_view(surface, *this, target);
        detail::write_text_file(filepath, detail::build_pdf_document(width, height, surface.finish()));
        return;
      }
      default:
        throw std::runtime_error("plotism::View4D::save: unsupported output format");
    }
  }

  std::string title_{};
  Style style_{};
  View4DLayout layout_{View4DLayout::Grid};
  std::vector<View4DPanel> panels_{};
};

namespace detail {

inline std::pair<std::size_t, std::size_t> grid_shape_4d(std::size_t count) {
  if (count <= 1) {
    return {1, 1};
  }
  const std::size_t cols = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(count))));
  const std::size_t rows = static_cast<std::size_t>(std::ceil(static_cast<double>(count) / static_cast<double>(cols)));
  return {rows, cols};
}

inline void render_4d_view(RenderSurface2D& surface, const View4D& view, const RenderTarget& target) {
  if (target.width <= 160 || target.height <= 160) {
    throw std::invalid_argument("plotism::View4D::save: width/height too small");
  }

  ResolvedTheme theme = resolve_theme(view.view_style());
  if (!view.panels().empty()) {
    theme = resolve_theme(view.panels().front().scene.view_data().style);
  }

  surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                    FillStyle2D{.color = theme.page_background, .alpha = 1.0});

  const double title_band = view.title().empty() ? 20.0 : 64.0;
  if (!view.title().empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, 30.0, view.title(),
                 TextStyle2D{
                     .color = theme.title_color,
                     .size = 24.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }

  if (view.panels().empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, static_cast<double>(target.height) * 0.5, "No 4D panels",
                 TextStyle2D{
                     .color = theme.tick_label_color,
                     .size = 14.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
    return;
  }

  if (view.layout() == View4DLayout::Single || view.panels().size() == 1) {
    const auto& panel = view.panels().front();
    SceneRender3DOptions options;
    options.clear_background = false;
    options.show_title = view.title().empty();
    options.title_override = view.title().empty()
                                 ? std::optional<std::string>{panel.title.empty() ? panel.scene.view_data().title : panel.title}
                                 : std::optional<std::string>{view.title()};
    render_3d_scene_with_options(surface, panel.scene.view_data(), target, options);
    return;
  }

  const auto [rows, cols] = grid_shape_4d(view.panels().size());
  const double outer_margin = 26.0;
  const double gap = 22.0;
  const double usable_width = static_cast<double>(target.width) - outer_margin * 2.0 - gap * static_cast<double>(cols - 1);
  const double usable_height =
      static_cast<double>(target.height) - title_band - outer_margin - gap * static_cast<double>(rows - 1);
  const double panel_width = usable_width / static_cast<double>(cols);
  const double panel_height = usable_height / static_cast<double>(rows);

  for (std::size_t index = 0; index < view.panels().size(); ++index) {
    const std::size_t row = index / cols;
    const std::size_t col = index % cols;
    const double left = outer_margin + static_cast<double>(col) * (panel_width + gap);
    const double top = title_band + static_cast<double>(row) * (panel_height + gap);
    SceneRender3DOptions options;
    options.viewport = SceneViewport3D{
        .left = left,
        .top = top,
        .width = panel_width,
        .height = panel_height,
    };
    options.clear_background = false;
    options.show_legend = false;
    options.show_colorbar = false;
    options.title_font_size = 13.0;
    options.max_ticks_per_axis = 4;
    const auto& panel = view.panels()[index];
    const bool bottom_row = row + 1 == rows;
    const bool left_col = col == 0;
    const bool right_col = col + 1 == cols || index + 1 == view.panels().size();
    options.show_x_axis_label = bottom_row;
    options.show_y_axis_label = left_col;
    options.show_z_axis_label = right_col;
    options.title_override = panel.title.empty() ? std::optional<std::string>{panel.scene.view_data().title}
                                                 : std::optional<std::string>{panel.title};
    render_3d_scene_with_options(surface, panel.scene.view_data(), target, options);
  }
}

} // namespace detail

} // namespace pluss::plotism
