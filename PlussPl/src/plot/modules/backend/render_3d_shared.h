#pragma once

#include <array>
#include <stdexcept>

#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

inline void render_3d_scene(RenderSurface2D& surface, const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save: width/height too small");
  }

  surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                    FillStyle2D{.color = "#f7f8fb", .alpha = 1.0});
  if (!fig.title.empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, 28.0, fig.title,
                 TextStyle2D{
                     .color = "#111827",
                     .size = 22.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                 });
  }

  const double left = 110.0;
  const double right = static_cast<double>(target.width - 90);
  const double top = 84.0;
  const double bottom = static_cast<double>(target.height - 96);
  const double pw = std::max(1.0, right - left);
  const double ph = std::max(1.0, bottom - top);

  surface.fill_rect(left, top, pw, ph, FillStyle2D{.color = "white", .alpha = 1.0}, 14.0);
  surface.stroke_rect(left, top, pw, ph,
                      StrokeStyle2D{.color = "#d9dfeb", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"},
                      14.0);

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = axis_transform_2d(t.x[i], fig.x_axis);
      const double ty = axis_transform_2d(t.y[i], fig.y_axis);
      const double tz = axis_transform_2d(t.z[i], fig.z_axis);
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

  if (fig.x_axis.min.has_value()) xmin = axis_transform_2d(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = axis_transform_2d(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = axis_transform_2d(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = axis_transform_2d(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = axis_transform_2d(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = axis_transform_2d(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = ensure_limits_2d(xmin, xmax);
  std::tie(ymin, ymax) = ensure_limits_2d(ymin, ymax);
  std::tie(zmin, zmax) = ensure_limits_2d(zmin, zmax);

  const auto project_normalized = [&](double xn, double yn, double zn) {
    if (fig.x_axis.inverted) xn = 1.0 - xn;
    if (fig.y_axis.inverted) yn = 1.0 - yn;
    if (fig.z_axis.inverted) zn = 1.0 - zn;
    const double px = left + (0.10 + xn * 0.60 + zn * 0.22) * pw;
    const double py = top + (0.90 - yn * 0.62 - zn * 0.24) * ph;
    return Point2D{px, py};
  };

  const auto project = [&](double x, double y, double z) {
    const double tx = axis_transform_2d(x, fig.x_axis);
    const double ty = axis_transform_2d(y, fig.y_axis);
    const double tz = axis_transform_2d(z, fig.z_axis);
    if (!std::isfinite(tx) || !std::isfinite(ty) || !std::isfinite(tz)) {
      return Point2D{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
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
    const auto p1 = project_normalized(corners[edge.first][0], corners[edge.first][1], corners[edge.first][2]);
    const auto p2 = project_normalized(corners[edge.second][0], corners[edge.second][1], corners[edge.second][2]);
    const bool primary = edge == std::pair<int, int>{0, 1} || edge == std::pair<int, int>{0, 3} ||
                         edge == std::pair<int, int>{0, 4};
    surface.stroke_line(p1.x, p1.y, p2.x, p2.y,
                        StrokeStyle2D{
                            .color = primary ? "#475569" : "#d7deea",
                            .line_width = primary ? 1.6 : 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });
  }

  const auto x_ticks = resolve_ticks_2d(fig.x_axis, xmin, xmax);
  const auto y_ticks = resolve_ticks_2d(fig.y_axis, ymin, ymax);
  const auto z_ticks = resolve_ticks_2d(fig.z_axis, zmin, zmax);

  for (const auto& tick : x_ticks) {
    const auto p = project_normalized((tick.position - xmin) / (xmax - xmin), 0.0, 0.0);
    surface.stroke_line(p.x, p.y, p.x, p.y + 6.0,
                        StrokeStyle2D{.color = "#64748b", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
    surface.text(p.x, p.y + 20.0, tick.label,
                 TextStyle2D{
                     .color = "#5b677a",
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                 });
  }
  for (const auto& tick : y_ticks) {
    const auto p = project_normalized(0.0, (tick.position - ymin) / (ymax - ymin), 0.0);
    surface.stroke_line(p.x - 6.0, p.y, p.x, p.y,
                        StrokeStyle2D{.color = "#64748b", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
    surface.text(p.x - 10.0, p.y + 3.5, tick.label,
                 TextStyle2D{
                     .color = "#5b677a",
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                 });
  }
  for (const auto& tick : z_ticks) {
    const auto p = project_normalized(0.0, 0.0, (tick.position - zmin) / (zmax - zmin));
    surface.stroke_line(p.x - 5.0, p.y + 4.0, p.x, p.y,
                        StrokeStyle2D{.color = "#64748b", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
    surface.text(p.x - 8.0, p.y - 6.0, tick.label,
                 TextStyle2D{
                     .color = "#5b677a",
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                 });
  }

  if (!fig.x_axis.label.empty()) {
    const auto p = project_normalized(1.0, 0.0, 0.0);
    surface.text(p.x + 14.0, p.y + 22.0, fig.x_axis.label,
                 TextStyle2D{.color = "#334155", .size = 12.0, .alpha = 1.0, .bold = false,
                             .anchor = TextAnchor2D::Left, .orientation = TextOrientation2D::Horizontal});
  }
  if (!fig.y_axis.label.empty()) {
    const auto p = project_normalized(0.0, 1.0, 0.0);
    surface.text(p.x - 18.0, p.y - 12.0, fig.y_axis.label,
                 TextStyle2D{.color = "#334155", .size = 12.0, .alpha = 1.0, .bold = false,
                             .anchor = TextAnchor2D::Right, .orientation = TextOrientation2D::Horizontal});
  }
  if (!fig.z_axis.label.empty()) {
    const auto p = project_normalized(0.0, 0.0, 1.0);
    surface.text(p.x - 8.0, p.y - 14.0, fig.z_axis.label,
                 TextStyle2D{.color = "#334155", .size = 12.0, .alpha = 1.0, .bold = false,
                             .anchor = TextAnchor2D::Right, .orientation = TextOrientation2D::Horizontal});
  }

  const auto& palette = detail::default_palette();
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    const auto& t = fig.traces[ti];
    const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));

    if (t.kind == Trace3DKind::Line3) {
      std::vector<Point2D> points;
      points.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        const auto p = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
          continue;
        }
        points.push_back(p);
      }
      surface.stroke_polyline(points,
                              StrokeStyle2D{
                                  .color = color,
                                  .line_width = t.style.line_width,
                                  .alpha = t.style.alpha,
                                  .line_style = t.style.line_style,
                              });
      if (canonical_marker_name(t.style.marker, false) != "none") {
        for (const auto& point : points) {
          draw_marker(surface, point.x, point.y, t.style.marker, 3.0, color, t.style.alpha);
        }
      }
    } else {
      const std::string marker = canonical_marker_name(t.style.marker, true);
      const double size = (t.kind == Trace3DKind::Surface) ? 2.2 : 3.0;
      for (std::size_t i = 0; i < n; ++i) {
        const auto p = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
          continue;
        }
        draw_marker(surface, p.x, p.y, marker, size, color, t.style.alpha);
      }
    }
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    std::size_t longest = fig.legend.title.size();
    for (const auto& trace : fig.traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !fig.legend.title.empty();
    const double legend_width = legend_width_hint_2d(fig.traces.size(), longest, has_title);
    const double legend_height = legend_height_hint_2d(fig.traces.size(), has_title);
    const auto [lx, ly] = map_legend_anchor_2d(fig.legend.location, left, right, top, bottom, legend_width, legend_height);

    surface.fill_rect(lx, ly, legend_width, legend_height, FillStyle2D{.color = "white", .alpha = 0.92}, 8.0);
    surface.stroke_rect(lx, ly, legend_width, legend_height,
                        StrokeStyle2D{.color = "#cfd6e3", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"},
                        8.0);
    double y = ly + 14.0;
    if (has_title) {
      surface.text(lx + 10.0, y, fig.legend.title,
                   TextStyle2D{
                       .color = "#1f2937",
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = true,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                   });
      y += 14.0;
    }
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      const auto& t = fig.traces[ti];
      const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      surface.stroke_line(lx + 8.0, sample_y, lx + 26.0, sample_y,
                          StrokeStyle2D{
                              .color = color,
                              .line_width = 2.0,
                              .alpha = 1.0,
                              .line_style = t.style.line_style,
                          });
      const std::string legend_marker =
          (t.kind == Trace3DKind::Line3) ? canonical_marker_name(t.style.marker, false)
                                         : canonical_marker_name(t.style.marker, true);
      if (legend_marker != "none") {
        draw_marker(surface, lx + 17.0, sample_y, legend_marker, 2.7, color, t.style.alpha);
      }
      surface.text(lx + 32.0, sample_y + 4.0, t.name.empty() ? std::string("trace") : t.name,
                   TextStyle2D{
                       .color = "#334155",
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                   });
    }
  }
}

} // namespace detail
} // namespace pluss::plotism
