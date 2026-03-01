#pragma once

#include <array>

#include "plot/modules/backend/svg/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_svg_3d(const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save_svg: width/height too small");
  }

  std::ostringstream svg;
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << target.width << "\" height=\""
      << target.height << "\" viewBox=\"0 0 " << target.width << " " << target.height << "\">\n";
  svg << "<rect x=\"0\" y=\"0\" width=\"" << target.width << "\" height=\"" << target.height
      << "\" fill=\"#f7f8fb\"/>\n";
  if (!fig.title.empty()) {
    svg << "<text x=\"" << (target.width / 2)
        << "\" y=\"28\" text-anchor=\"middle\" font-size=\"22\" font-weight=\"600\" font-family=\"sans-serif\">"
        << detail::escape_xml(fig.title) << "</text>\n";
  }

  const int left = 110;
  const int right = target.width - 90;
  const int top = 84;
  const int bottom = target.height - 96;
  const int pw = std::max(1, right - left);
  const int ph = std::max(1, bottom - top);

  svg << "<rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << pw << "\" height=\"" << ph
      << "\" rx=\"14\" fill=\"white\" stroke=\"#d9dfeb\"/>\n";

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = axis_transform(t.x[i], fig.x_axis);
      const double ty = axis_transform(t.y[i], fig.y_axis);
      const double tz = axis_transform(t.z[i], fig.z_axis);
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

  if (fig.x_axis.min.has_value()) xmin = axis_transform(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = axis_transform(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = axis_transform(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = axis_transform(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = axis_transform(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = axis_transform(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = ensure_limits(xmin, xmax);
  std::tie(ymin, ymax) = ensure_limits(ymin, ymax);
  std::tie(zmin, zmax) = ensure_limits(zmin, zmax);

  const auto project_normalized = [&](double xn, double yn, double zn) {
    if (fig.x_axis.inverted) xn = 1.0 - xn;
    if (fig.y_axis.inverted) yn = 1.0 - yn;
    if (fig.z_axis.inverted) zn = 1.0 - zn;
    const double px = static_cast<double>(left) + (0.10 + xn * 0.60 + zn * 0.22) * static_cast<double>(pw);
    const double py = static_cast<double>(top) + (0.90 - yn * 0.62 - zn * 0.24) * static_cast<double>(ph);
    return std::pair<double, double>{px, py};
  };

  const auto project = [&](double x, double y, double z) {
    const double tx = axis_transform(x, fig.x_axis);
    const double ty = axis_transform(y, fig.y_axis);
    const double tz = axis_transform(z, fig.z_axis);
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
    const bool primary = edge == std::pair<int, int>{0, 1} || edge == std::pair<int, int>{0, 3} || edge == std::pair<int, int>{0, 4};
    svg << "<line x1=\"" << format_double(x1, 6) << "\" y1=\"" << format_double(y1, 6)
        << "\" x2=\"" << format_double(x2, 6) << "\" y2=\"" << format_double(y2, 6)
        << "\" stroke=\"" << (primary ? "#475569" : "#d7deea")
        << "\" stroke-width=\"" << (primary ? "1.6" : "1.0") << "\"/>\n";
  }

  const auto x_ticks = make_ticks(xmin, xmax, 4);
  const auto y_ticks = make_ticks(ymin, ymax, 4);
  const auto z_ticks = make_ticks(zmin, zmax, 4);

  for (double tick : x_ticks) {
    const double xn = (tick - xmin) / (xmax - xmin);
    const auto [px, py] = project_normalized(xn, 0.0, 0.0);
    svg << "<line x1=\"" << format_double(px, 6) << "\" y1=\"" << format_double(py, 6)
        << "\" x2=\"" << format_double(px, 6) << "\" y2=\"" << format_double(py + 6.0, 6)
        << "\" stroke=\"#64748b\" stroke-width=\"1\"/>\n";
    svg << "<text x=\"" << format_double(px, 6) << "\" y=\"" << format_double(py + 20.0, 6)
        << "\" text-anchor=\"middle\" font-size=\"10\" font-family=\"sans-serif\" fill=\"#5b677a\">"
        << escape_xml(format_tick_label(tick, fig.x_axis)) << "</text>\n";
  }
  for (double tick : y_ticks) {
    const double yn = (tick - ymin) / (ymax - ymin);
    const auto [px, py] = project_normalized(0.0, yn, 0.0);
    svg << "<line x1=\"" << format_double(px - 6.0, 6) << "\" y1=\"" << format_double(py, 6)
        << "\" x2=\"" << format_double(px, 6) << "\" y2=\"" << format_double(py, 6)
        << "\" stroke=\"#64748b\" stroke-width=\"1\"/>\n";
    svg << "<text x=\"" << format_double(px - 10.0, 6) << "\" y=\"" << format_double(py + 3.5, 6)
        << "\" text-anchor=\"end\" font-size=\"10\" font-family=\"sans-serif\" fill=\"#5b677a\">"
        << escape_xml(format_tick_label(tick, fig.y_axis)) << "</text>\n";
  }
  for (double tick : z_ticks) {
    const double zn = (tick - zmin) / (zmax - zmin);
    const auto [px, py] = project_normalized(0.0, 0.0, zn);
    svg << "<line x1=\"" << format_double(px - 5.0, 6) << "\" y1=\"" << format_double(py + 4.0, 6)
        << "\" x2=\"" << format_double(px, 6) << "\" y2=\"" << format_double(py, 6)
        << "\" stroke=\"#64748b\" stroke-width=\"1\"/>\n";
    svg << "<text x=\"" << format_double(px - 8.0, 6) << "\" y=\"" << format_double(py - 6.0, 6)
        << "\" text-anchor=\"end\" font-size=\"10\" font-family=\"sans-serif\" fill=\"#5b677a\">"
        << escape_xml(format_tick_label(tick, fig.z_axis)) << "</text>\n";
  }

  if (!fig.x_axis.label.empty()) {
    const auto [px, py] = project_normalized(1.0, 0.0, 0.0);
    svg << "<text x=\"" << format_double(px + 14.0, 6) << "\" y=\"" << format_double(py + 22.0, 6)
        << "\" font-size=\"12\" font-family=\"sans-serif\" fill=\"#334155\">"
        << escape_xml(fig.x_axis.label) << "</text>\n";
  }
  if (!fig.y_axis.label.empty()) {
    const auto [px, py] = project_normalized(0.0, 1.0, 0.0);
    svg << "<text x=\"" << format_double(px - 18.0, 6) << "\" y=\"" << format_double(py - 12.0, 6)
        << "\" text-anchor=\"end\" font-size=\"12\" font-family=\"sans-serif\" fill=\"#334155\">"
        << escape_xml(fig.y_axis.label) << "</text>\n";
  }
  if (!fig.z_axis.label.empty()) {
    const auto [px, py] = project_normalized(0.0, 0.0, 1.0);
    svg << "<text x=\"" << format_double(px - 8.0, 6) << "\" y=\"" << format_double(py - 14.0, 6)
        << "\" text-anchor=\"end\" font-size=\"12\" font-family=\"sans-serif\" fill=\"#334155\">"
        << escape_xml(fig.z_axis.label) << "</text>\n";
  }

  const auto& palette = detail::default_palette();
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    const auto& t = fig.traces[ti];
    const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));

    if (t.kind == Trace3DKind::Line3) {
      svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << t.style.line_width
          << "\" stroke-opacity=\"" << t.style.alpha << "\" stroke-linejoin=\"round\" stroke-linecap=\"round\""
          << svg_dasharray(t.style.line_style, t.style.line_width) << " points=\"";
      std::vector<std::pair<double, double>> points;
      points.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        points.push_back({px, py});
        svg << format_double(px, 6) << "," << format_double(py, 6) << " ";
      }
      svg << "\"/>\n";
      if (normalized_marker(t.style.marker, false) != "none") {
        for (const auto& point : points) {
          append_svg_marker(svg, point.first, point.second, t.style.marker, 3.0, color, t.style.alpha);
        }
      }
    } else {
      const std::string marker = normalized_marker(t.style.marker, true);
      const double size = (t.kind == Trace3DKind::Surface) ? 2.2 : 3.0;
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        append_svg_marker(svg, px, py, marker, size, color, t.style.alpha);
      }
    }
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    std::size_t longest = fig.legend.title.size();
    for (const auto& trace : fig.traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !fig.legend.title.empty();
    const double legend_width = legend_width_hint(fig.traces.size(), longest, has_title);
    const double legend_height = legend_height_hint(fig.traces.size(), has_title);
    const auto [lx, ly] = map_legend_anchor(
        fig.legend.location,
        static_cast<double>(left),
        static_cast<double>(right),
        static_cast<double>(top),
        static_cast<double>(bottom),
        legend_width,
        legend_height);
    svg << "<rect x=\"" << lx << "\" y=\"" << ly << "\" width=\"" << legend_width
        << "\" height=\"" << legend_height
        << "\" rx=\"8\" fill=\"white\" fill-opacity=\"0.92\" stroke=\"#cfd6e3\"/>\n";
    double y = ly + 14.0;
    if (has_title) {
      svg << "<text x=\"" << (lx + 10.0)
          << "\" y=\"" << y
          << "\" font-size=\"11\" font-weight=\"600\" font-family=\"sans-serif\" fill=\"#1f2937\">"
          << escape_xml(fig.legend.title) << "</text>\n";
      y += 14.0;
    }
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      const auto& t = fig.traces[ti];
      const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      svg << "<line x1=\"" << (lx + 8.0) << "\" y1=\"" << sample_y << "\" x2=\"" << (lx + 26.0)
          << "\" y2=\"" << sample_y << "\" stroke=\"" << color << "\" stroke-width=\"2\""
          << svg_dasharray(t.style.line_style, 2.0) << "/>\n";
      const std::string legend_marker =
          (t.kind == Trace3DKind::Line3) ? normalized_marker(t.style.marker, false) : normalized_marker(t.style.marker, true);
      if (legend_marker != "none") {
        append_svg_marker(svg, lx + 17.0, sample_y, legend_marker, 2.7, color, t.style.alpha);
      }
      svg << "<text x=\"" << (lx + 32.0)
          << "\" y=\"" << (sample_y + 4.0)
          << "\" font-size=\"11\" font-family=\"sans-serif\" fill=\"#334155\">"
          << detail::escape_xml(t.name.empty() ? std::string("trace") : t.name) << "</text>\n";
    }
  }

  svg << "</svg>\n";
  return svg.str();
}
} // namespace detail
} // namespace pluss::plotism
