#pragma once

#include <array>

#include "plot/modules/backend/pdf/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_pdf_3d(const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save_pdf: width/height too small");
  }

  PdfPainter painter(target.width, target.height);
  const PdfRgb black = pdf_parse_color("black");
  const PdfRgb white = pdf_parse_color("white");
  const PdfRgb page_bg = pdf_parse_color("#f7f8fb");
  const PdfRgb border = pdf_parse_color("#d9dfeb");
  const PdfRgb secondary = pdf_parse_color("#d7deea");
  const PdfRgb primary = pdf_parse_color("#475569");
  const PdfRgb tick_color = pdf_parse_color("#64748b");
  const PdfRgb label_color = pdf_parse_color("#5b677a");

  painter.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height), page_bg);
  if (!fig.title.empty()) {
    painter.text_center(static_cast<double>(target.width) * 0.5, 28.0, 22.0, fig.title, black);
  }

  const int left = 110;
  const int right = target.width - 90;
  const int top = 84;
  const int bottom = target.height - 96;
  const int pw = std::max(1, right - left);
  const int ph = std::max(1, bottom - top);

  painter.fill_rect(static_cast<double>(left), static_cast<double>(top), static_cast<double>(pw), static_cast<double>(ph), white);
  painter.stroke_rect(static_cast<double>(left), static_cast<double>(top), static_cast<double>(pw), static_cast<double>(ph), border, 1.0);

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = pdf_axis_transform(t.x[i], fig.x_axis);
      const double ty = pdf_axis_transform(t.y[i], fig.y_axis);
      const double tz = pdf_axis_transform(t.z[i], fig.z_axis);
      if (std::isfinite(tx)) {
        xmin = std::min(xmin, tx);
        xmax = std::max(xmax, tx);
      }
      if (std::isfinite(ty)) {
        ymin = std::min(ymin, ty);
        ymax = std::max(ymax, ty);
      }
      if (std::isfinite(tz)) {
        zmin = std::min(zmin, tz);
        zmax = std::max(zmax, tz);
      }
    }
  }

  if (fig.x_axis.min.has_value()) xmin = pdf_axis_transform(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = pdf_axis_transform(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = pdf_axis_transform(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = pdf_axis_transform(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = pdf_axis_transform(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = pdf_axis_transform(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = pdf_ensure_limits(xmin, xmax);
  std::tie(ymin, ymax) = pdf_ensure_limits(ymin, ymax);
  std::tie(zmin, zmax) = pdf_ensure_limits(zmin, zmax);

  const auto project_normalized = [&](double xn, double yn, double zn) {
    if (fig.x_axis.inverted) xn = 1.0 - xn;
    if (fig.y_axis.inverted) yn = 1.0 - yn;
    if (fig.z_axis.inverted) zn = 1.0 - zn;
    const double px = static_cast<double>(left) + (0.10 + xn * 0.60 + zn * 0.22) * static_cast<double>(pw);
    const double py = static_cast<double>(top) + (0.90 - yn * 0.62 - zn * 0.24) * static_cast<double>(ph);
    return std::pair<double, double>{px, py};
  };

  const auto project = [&](double x, double y, double z) {
    const double tx = pdf_axis_transform(x, fig.x_axis);
    const double ty = pdf_axis_transform(y, fig.y_axis);
    const double tz = pdf_axis_transform(z, fig.z_axis);
    if (!std::isfinite(tx) || !std::isfinite(ty) || !std::isfinite(tz)) {
      return std::pair<double, double>{std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN()};
    }
    return project_normalized((tx - xmin) / (xmax - xmin), (ty - ymin) / (ymax - ymin), (tz - zmin) / (zmax - zmin));
  };

  const std::array<std::array<double, 3>, 8> corners{{
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.0, 1.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0},
      {1.0, 0.0, 1.0},
      {1.0, 1.0, 1.0},
      {0.0, 1.0, 1.0},
  }};
  const std::array<std::pair<int, int>, 12> edges{{
      {0, 1}, {1, 2}, {2, 3}, {3, 0},
      {4, 5}, {5, 6}, {6, 7}, {7, 4},
      {0, 4}, {1, 5}, {2, 6}, {3, 7},
  }};

  for (const auto& edge : edges) {
    const auto [x1, y1] = project_normalized(corners[edge.first][0], corners[edge.first][1], corners[edge.first][2]);
    const auto [x2, y2] = project_normalized(corners[edge.second][0], corners[edge.second][1], corners[edge.second][2]);
    const bool is_primary = edge == std::pair<int, int>{0, 1} || edge == std::pair<int, int>{0, 3} || edge == std::pair<int, int>{0, 4};
    painter.stroke_line(x1, y1, x2, y2, is_primary ? primary : secondary, is_primary ? 1.6 : 1.0);
  }

  const auto x_ticks = pdf_make_ticks(xmin, xmax, 4);
  const auto y_ticks = pdf_make_ticks(ymin, ymax, 4);
  const auto z_ticks = pdf_make_ticks(zmin, zmax, 4);

  for (double tick : x_ticks) {
    const double xn = (tick - xmin) / (xmax - xmin);
    const auto [px, py] = project_normalized(xn, 0.0, 0.0);
    painter.stroke_line(px, py, px, py + 6.0, tick_color, 1.0);
    painter.text_center(px, py + 20.0, 10.0, pdf_format_tick_label(tick, fig.x_axis), label_color);
  }
  for (double tick : y_ticks) {
    const double yn = (tick - ymin) / (ymax - ymin);
    const auto [px, py] = project_normalized(0.0, yn, 0.0);
    painter.stroke_line(px - 6.0, py, px, py, tick_color, 1.0);
    painter.text_left(px - 34.0, py + 3.5, 10.0, pdf_format_tick_label(tick, fig.y_axis), label_color);
  }
  for (double tick : z_ticks) {
    const double zn = (tick - zmin) / (zmax - zmin);
    const auto [px, py] = project_normalized(0.0, 0.0, zn);
    painter.stroke_line(px - 5.0, py + 4.0, px, py, tick_color, 1.0);
    painter.text_left(px - 36.0, py - 6.0, 10.0, pdf_format_tick_label(tick, fig.z_axis), label_color);
  }

  if (!fig.x_axis.label.empty()) {
    const auto [px, py] = project_normalized(1.0, 0.0, 0.0);
    painter.text_left(px + 14.0, py + 22.0, 12.0, fig.x_axis.label, black);
  }
  if (!fig.y_axis.label.empty()) {
    const auto [px, py] = project_normalized(0.0, 1.0, 0.0);
    painter.text_left(px - 48.0, py - 12.0, 12.0, fig.y_axis.label, black);
  }
  if (!fig.z_axis.label.empty()) {
    const auto [px, py] = project_normalized(0.0, 0.0, 1.0);
    painter.text_left(px - 38.0, py - 14.0, 12.0, fig.z_axis.label, black);
  }

  const auto& palette = detail::default_palette();
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    const auto& t = fig.traces[ti];
    const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
    const PdfRgb color = pdf_parse_color(color_hex);
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));

    if (t.kind == Trace3DKind::Line3) {
      std::vector<std::pair<double, double>> points;
      points.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        points.push_back({px, py});
      }
      painter.stroke_polyline(points, color, t.style.line_width, pdf_dash_pattern(t.style.line_style, t.style.line_width));
      if (pdf_normalized_marker(t.style.marker, false) != "none") {
        for (const auto& point : points) {
          draw_pdf_marker(painter, point.first, point.second, t.style.marker, 3.0, color);
        }
      }
    } else {
      const std::string marker = pdf_normalized_marker(t.style.marker, true);
      const double size = (t.kind == Trace3DKind::Surface) ? 2.2 : 3.0;
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        draw_pdf_marker(painter, px, py, marker, size, color);
      }
    }
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    std::size_t longest = fig.legend.title.size();
    for (const auto& trace : fig.traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !fig.legend.title.empty();
    const double legend_width = pdf_legend_width_hint(fig.traces.size(), longest, has_title);
    const double legend_height = pdf_legend_height_hint(fig.traces.size(), has_title);
    const auto [lx, ly] = pdf_map_legend_anchor(
        fig.legend.location,
        static_cast<double>(left),
        static_cast<double>(right),
        static_cast<double>(top),
        static_cast<double>(bottom),
        legend_width,
        legend_height);

    painter.fill_rect(lx, ly, legend_width, legend_height, white);
    painter.stroke_rect(lx, ly, legend_width, legend_height, pdf_parse_color("#cfd6e3"), 1.0);
    double y = ly + 14.0;
    if (has_title) {
      painter.text_left(lx + 10.0, y, 11.0, fig.legend.title, black);
      y += 14.0;
    }
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      const auto& t = fig.traces[ti];
      const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      const PdfRgb color = pdf_parse_color(color_hex);
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      painter.stroke_line(lx + 8.0, sample_y, lx + 26.0, sample_y, color, 2.0, pdf_dash_pattern(t.style.line_style, 2.0));
      const std::string legend_marker =
          (t.kind == Trace3DKind::Line3) ? pdf_normalized_marker(t.style.marker, false) : pdf_normalized_marker(t.style.marker, true);
      if (legend_marker != "none") {
        draw_pdf_marker(painter, lx + 17.0, sample_y, legend_marker, 2.7, color);
      }
      painter.text_left(lx + 32.0, sample_y + 4.0, 11.0, t.name.empty() ? std::string("trace") : t.name, black);
    }
  }

  return build_pdf_document(target.width, target.height, painter.finish());
}

} // namespace detail
} // namespace pluss::plotism
