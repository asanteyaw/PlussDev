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

struct View4DSharedLegend {
  Legend legend{};
  std::vector<Trace3D> traces{};
};

struct View4DSharedColorbar {
  Colorbar colorbar{};
  Trace2D trace{};
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

  View4D& shared_legend(View4DSharedLegend legend) {
    shared_legend_ = std::move(legend);
    return *this;
  }

  View4D& shared_colorbar(View4DSharedColorbar colorbar) {
    shared_colorbar_ = std::move(colorbar);
    return *this;
  }

  const std::string& title() const noexcept { return title_; }
  const Style& view_style() const noexcept { return style_; }
  View4DLayout layout() const noexcept { return layout_; }
  const std::vector<View4DPanel>& panels() const noexcept { return panels_; }
  const std::optional<View4DSharedLegend>& shared_legend() const noexcept { return shared_legend_; }
  const std::optional<View4DSharedColorbar>& shared_colorbar() const noexcept { return shared_colorbar_; }
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
    save_(path, width, height, 144.0, format);
    return detail::open_plot_viewer(title_, path, block, width, height);
  }

  bool show_popup(OutputFormat format = OutputFormat::Svg,
                  std::int32_t width = 1400,
                  std::int32_t height = 960) const {
    return show_window(true, format, width, height);
  }

  void save_svg(std::string_view filepath,
                std::int32_t width = 1400,
                std::int32_t height = 960,
                double dpi = 144.0) const {
    save_(filepath, width, height, dpi, OutputFormat::Svg);
  }

  void save_png(std::string_view filepath,
                std::int32_t width = 1400,
                std::int32_t height = 960,
                double dpi = 144.0) const {
    save_(filepath, width, height, dpi, OutputFormat::Png);
  }

  void save_pdf(std::string_view filepath,
                std::int32_t width = 1400,
                std::int32_t height = 960,
                double dpi = 144.0) const {
    save_(filepath, width, height, dpi, OutputFormat::Pdf);
  }

private:
  void save_(std::string_view filepath, std::int32_t width, std::int32_t height, double dpi, OutputFormat format) const {
    const RenderTarget target{.path = filepath, .width = width, .height = height, .dpi = dpi, .format = format};
    switch (format) {
      case OutputFormat::Svg: {
        detail::SvgRenderSurface2D surface;
        detail::render_4d_view(surface, *this, target);
        std::string svg;
        svg.reserve(2048);
        const double physical_width = static_cast<double>(width) / std::max(1.0, dpi);
        const double physical_height = static_cast<double>(height) / std::max(1.0, dpi);
        svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"";
        svg += detail::format_double(physical_width, 6);
        svg += "in\" height=\"";
        svg += detail::format_double(physical_height, 6);
        svg += "in\"";
        svg += " data-plusspl-dpi=\"";
        svg += detail::format_double(dpi, 6);
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
        detail::RasterSurface2D surface(width, height, detail::recommended_png_supersample(width, height), dpi);
        detail::render_4d_view(surface, *this, target);
        detail::write_text_file(filepath, surface.finish());
        return;
      }
      case OutputFormat::Pdf: {
        detail::PdfRenderSurface2D surface(width, height);
        detail::render_4d_view(surface, *this, target);
        detail::write_text_file(filepath, detail::build_pdf_document(width, height, surface.finish(), dpi));
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
  std::optional<View4DSharedLegend> shared_legend_{};
  std::optional<View4DSharedColorbar> shared_colorbar_{};
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
  double shared_legend_width = 0.0;
  double shared_legend_height = 0.0;
  if (view.shared_legend().has_value() && view.shared_legend()->legend.visible && !view.shared_legend()->traces.empty()) {
    std::size_t longest = view.shared_legend()->legend.title.size();
    for (const auto& trace : view.shared_legend()->traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !view.shared_legend()->legend.title.empty();
    shared_legend_width = legend_width_hint_2d(view.shared_legend()->traces.size(), longest, has_title);
    shared_legend_height = legend_height_hint_2d(view.shared_legend()->traces.size(), has_title);
  }
  const bool shared_colorbar_visible = view.shared_colorbar().has_value() &&
                                       view.shared_colorbar()->colorbar.visible &&
                                       !view.shared_colorbar()->trace.values.empty();
  const double shared_colorbar_width = shared_colorbar_visible ? 56.0 : 0.0;
  const double shared_aux_width = std::max(shared_legend_width, shared_colorbar_width);
  const double shared_aux_gap = shared_aux_width > 0.0 ? 24.0 : 0.0;
  const double usable_width = static_cast<double>(target.width) - outer_margin * 2.0 - gap * static_cast<double>(cols - 1) -
                              shared_aux_width - shared_aux_gap;
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
    options.max_ticks_per_axis = std::max(2, static_cast<int>(std::llround(std::sqrt(std::max(1.0, panel_width * panel_height)) / 20.0)));
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

  if (shared_aux_width <= 0.0) {
    return;
  }

  const double aux_left = outer_margin + static_cast<double>(cols) * panel_width + static_cast<double>(cols - 1) * gap + shared_aux_gap;
  double aux_top = title_band + 6.0;
  if (shared_colorbar_visible) {
    const double colorbar_h = std::clamp(panel_height * static_cast<double>(rows) + gap * static_cast<double>(rows - 1) - 12.0,
                                         120.0,
                                         static_cast<double>(target.height) - aux_top - outer_margin);
    const ColorbarArtist2D colorbar_artist{};
    colorbar_artist.draw(surface,
                         aux_left,
                         aux_top,
                         shared_colorbar_width,
                         colorbar_h,
                         view.shared_colorbar()->trace,
                         view.shared_colorbar()->colorbar,
                         theme);
    aux_top += colorbar_h + 18.0;
  }

  if (shared_legend_width > 0.0) {
    surface.fill_rect(aux_left, aux_top, shared_legend_width, shared_legend_height,
                      FillStyle2D{.color = theme.legend_background, .alpha = 0.92},
                      8.0);
    surface.stroke_rect(aux_left, aux_top, shared_legend_width, shared_legend_height,
                        StrokeStyle2D{
                            .color = theme.legend_border,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        },
                        8.0);
    double y = aux_top + 14.0;
    if (!view.shared_legend()->legend.title.empty()) {
      surface.text(aux_left + 10.0, y, view.shared_legend()->legend.title,
                   TextStyle2D{
                       .color = theme.legend_title_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = true,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
      y += 14.0;
    }
    const LegendArtist3D legend_artist{};
    for (std::size_t ti = 0; ti < view.shared_legend()->traces.size(); ++ti) {
      Trace3D styled_trace = view.shared_legend()->traces[ti];
      apply_trace_style_defaults(styled_trace, theme, ti);
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      legend_artist.draw_sample(surface, styled_trace, aux_left + 8.0, aux_left + 26.0, sample_y, theme);
      surface.text(aux_left + 32.0, sample_y + 4.0, styled_trace.name.empty() ? std::string("trace") : styled_trace.name,
                   TextStyle2D{
                       .color = theme.legend_text_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
    }
  }
}

} // namespace detail

} // namespace pluss::plotism
