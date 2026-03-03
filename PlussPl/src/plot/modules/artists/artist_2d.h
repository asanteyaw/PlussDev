#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

struct PlotTransform2D {
  double xmin{0.0};
  double xmax{1.0};
  double ymin{0.0};
  double ymax{1.0};
  double left{0.0};
  double right{1.0};
  double top{0.0};
  double bottom{1.0};
  const XAxis* x_axis{nullptr};
  const YAxis* y_axis{nullptr};

  double plot_width() const { return right - left; }
  double plot_height() const { return bottom - top; }

  double sx(double x) const {
    if (x_axis == nullptr) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    const double tx = x_axis->transform_value(x);
    if (!std::isfinite(tx)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    double xn = (tx - xmin) / (xmax - xmin);
    if (x_axis->inverted) {
      xn = 1.0 - xn;
    }
    return left + xn * plot_width();
  }

  double sy(double y) const {
    if (y_axis == nullptr) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    const double ty = y_axis->transform_value(y);
    if (!std::isfinite(ty)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    double yn = (ty - ymin) / (ymax - ymin);
    if (y_axis->inverted) {
      yn = 1.0 - yn;
    }
    return bottom - yn * plot_height();
  }
};

struct ArtistStyle2D {
  std::string color{};
  double line_width{1.5};
  double alpha{1.0};
  std::string line_style{"solid"};
  std::string marker{"none"};
  double marker_size{0.0};
};

inline ArtistStyle2D resolve_artist_style_2d(const Trace2D& trace, const std::string& fallback_color) {
  return ArtistStyle2D{
      .color = trace.style.color.empty() ? fallback_color : trace.style.color,
      .line_width = trace.style.line_width,
      .alpha = trace.style.alpha,
      .line_style = trace.style.line_style,
      .marker = trace.style.marker,
      .marker_size = trace.style.marker_size,
  };
}

inline double effective_marker_size_2d(const ArtistStyle2D& style, double fallback) {
  return style.marker_size > 0.0 ? style.marker_size : fallback;
}

inline double default_fill_alpha_2d(const Trace2D& trace) {
  if ((trace.kind == Trace2DKind::Area || trace.kind == Trace2DKind::Band) && trace.style.alpha >= 0.999) {
    return 0.22;
  }
  return trace.style.alpha;
}

inline std::pair<double, double> cell_bounds_2d(const std::vector<double>& coords, std::size_t index) {
  if (coords.empty() || index >= coords.size()) {
    return {0.0, 1.0};
  }
  if (coords.size() == 1) {
    return {coords.front() - 0.5, coords.front() + 0.5};
  }
  const double center = coords[index];
  const double lo =
      (index == 0) ? center - 0.5 * (coords[1] - coords[0]) : 0.5 * (coords[index - 1] + center);
  const double hi = (index + 1 == coords.size()) ? center + 0.5 * (center - coords[index - 1])
                                                 : 0.5 * (center + coords[index + 1]);
  return {lo, hi};
}

inline std::pair<double, double> bar_bounds_2d(const Trace2D& trace, std::size_t index) {
  if (index < trace.x.size() && trace.x2.size() == trace.x.size() && std::isfinite(trace.x2[index]) && trace.x2[index] > 0.0) {
    const double half_width = 0.5 * trace.x2[index];
    return {trace.x[index] - half_width, trace.x[index] + half_width};
  }
  return cell_bounds_2d(trace.x, index);
}

inline std::vector<Point2D> make_step_points_2d(const Trace2D& trace, const PlotTransform2D& transform) {
  std::vector<Point2D> points;
  const std::size_t n = std::min(trace.x.size(), trace.y.size());
  if (n == 0) {
    return points;
  }
  points.reserve(n * 2);
  for (std::size_t i = 0; i < n; ++i) {
    const double px = transform.sx(trace.x[i]);
    const double py = transform.sy(trace.y[i]);
    if (!std::isfinite(px) || !std::isfinite(py)) {
      continue;
    }
    if (points.empty()) {
      points.push_back({px, py});
      continue;
    }
    points.push_back({px, points.back().y});
    points.push_back({px, py});
  }
  return points;
}

inline std::string heatmap_color_2d(double value, double lo, double hi) {
  static const std::vector<std::string> palette{
      "#0b3c5d", "#166088", "#2d88a8", "#6fb1c7", "#e9eef2", "#f4c095", "#e67e5f", "#c44536", "#772e25"};
  if (!std::isfinite(value)) {
    return "#e5e7eb";
  }
  if (!(hi > lo)) {
    return palette[palette.size() / 2];
  }
  const double t = std::clamp((value - lo) / (hi - lo), 0.0, 1.0);
  const std::size_t idx = static_cast<std::size_t>(std::round(t * static_cast<double>(palette.size() - 1)));
  return palette[std::min(idx, palette.size() - 1)];
}

inline std::pair<double, double> finite_minmax_2d(const std::vector<double>& values) {
  double lo = std::numeric_limits<double>::infinity();
  double hi = -std::numeric_limits<double>::infinity();
  for (double value : values) {
    if (!std::isfinite(value)) {
      continue;
    }
    lo = std::min(lo, value);
    hi = std::max(hi, value);
  }
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    return {0.0, 1.0};
  }
  return {lo, hi};
}

class Artist2D {
public:
  virtual ~Artist2D() = default;
  virtual void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const = 0;
};

class LegendArtist2D {
public:
  void draw_sample(RenderSurface2D& surface,
                   const Trace2D& trace,
                   double x1,
                   double x2,
                   double y,
                   const std::string& color) const {
    if (trace.kind == Trace2DKind::Heatmap || trace.kind == Trace2DKind::Area || trace.kind == Trace2DKind::Band) {
      surface.fill_rect(x1, y - 4.0, x2 - x1, 8.0,
                        FillStyle2D{
                            .color = (trace.kind == Trace2DKind::Heatmap) ? std::string("#c44536") : color,
                            .alpha = (trace.kind == Trace2DKind::Heatmap) ? 1.0 : default_fill_alpha_2d(trace),
                        },
                        2.0);
      surface.stroke_rect(x1, y - 4.0, x2 - x1, 8.0,
                          StrokeStyle2D{.color = color, .line_width = 1.0, .alpha = 0.9, .line_style = "solid"},
                          2.0);
      return;
    }
    if (trace.kind == Trace2DKind::Annotation) {
      detail::draw_marker(surface, x1 + 4.0, y, "circle", 2.4, color, 1.0);
      surface.stroke_line(x1 + 6.0, y - 2.0, x2, y - 6.0,
                          StrokeStyle2D{.color = color, .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
      return;
    }
    surface.stroke_line(x1, y, x2, y,
                        StrokeStyle2D{
                            .color = color,
                            .line_width = 2.0,
                            .alpha = 1.0,
                            .line_style = trace.style.line_style,
                        });

    const std::string marker =
        (trace.kind == Trace2DKind::Scatter) ? detail::canonical_marker_name(trace.style.marker, true)
                                             : detail::canonical_marker_name(trace.style.marker, false);
    if (marker != "none") {
      detail::draw_marker(surface,
                          (x1 + x2) * 0.5,
                          y,
                          marker,
                          trace.style.marker_size > 0.0 ? trace.style.marker_size : 2.7,
                          color,
                          trace.style.alpha);
    }
  }
};

class ColorbarArtist2D {
public:
  void draw(RenderSurface2D& surface,
            double x,
            double y,
            double width,
            double height,
            const Trace2D& trace,
            const Colorbar& colorbar,
            const ResolvedTheme& theme) const {
    if (trace.kind != Trace2DKind::Heatmap || trace.values.empty()) {
      return;
    }

    const auto [vmin, vmax] = finite_minmax_2d(trace.values);
    const double bar_width = std::min(18.0, std::max(12.0, width * 0.34));
    const double bar_x = x + 6.0;
    const double bar_y = y + (colorbar.title.empty() ? 6.0 : 20.0);
    const double bar_h = std::max(36.0, height - (colorbar.title.empty() ? 12.0 : 30.0));
    const int steps = 36;

    surface.fill_rect(x, y, width, height, FillStyle2D{.color = theme.legend_background, .alpha = 0.96}, 8.0);
    surface.stroke_rect(x, y, width, height,
                        StrokeStyle2D{
                            .color = theme.legend_border,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        },
                        8.0);

    if (!colorbar.title.empty()) {
      surface.text(x + width * 0.5, y + 13.0, colorbar.title,
                   TextStyle2D{
                       .color = theme.legend_title_color,
                       .size = 10.0,
                       .alpha = 1.0,
                       .bold = true,
                       .anchor = TextAnchor2D::Center,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
    }

    for (int i = 0; i < steps; ++i) {
      const double t0 = static_cast<double>(i) / static_cast<double>(steps);
      const double t1 = static_cast<double>(i + 1) / static_cast<double>(steps);
      const double value = vmax - t0 * (vmax - vmin);
      const double yy = bar_y + t0 * bar_h;
      const double hh = std::max(1.0, (t1 - t0) * bar_h + 0.2);
      surface.fill_rect(bar_x, yy, bar_width, hh, FillStyle2D{.color = heatmap_color_2d(value, vmin, vmax), .alpha = 1.0});
    }
    surface.stroke_rect(bar_x, bar_y, bar_width, bar_h,
                        StrokeStyle2D{
                            .color = theme.plot_border,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });

    const std::array<double, 3> fractions{0.0, 0.5, 1.0};
    for (double fraction : fractions) {
      const double yy = bar_y + fraction * bar_h;
      const double value = vmax - fraction * (vmax - vmin);
      surface.stroke_line(bar_x + bar_width, yy, bar_x + bar_width + 4.0, yy,
                          StrokeStyle2D{
                              .color = theme.tick_color,
                              .line_width = 1.0,
                              .alpha = 1.0,
                              .line_style = "solid",
                          });
      surface.text(bar_x + bar_width + 8.0, yy + 3.5, detail::format_double(value, 4),
                   TextStyle2D{
                       .color = theme.tick_label_color,
                       .size = 9.0,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
    }
  }
};

class LineArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    std::vector<Point2D> points;
    const std::size_t n = std::min(trace.x.size(), trace.y.size());
    points.reserve(n);
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y[i]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      points.push_back({px, py});
    }
    surface.stroke_polyline(points,
                            StrokeStyle2D{
                                .color = style.color,
                                .line_width = style.line_width,
                                .alpha = style.alpha,
                                .line_style = style.line_style,
                            });
    if (detail::canonical_marker_name(style.marker, false) != "none") {
      for (const auto& point : points) {
        detail::draw_marker(surface,
                            point.x,
                            point.y,
                            style.marker,
                            effective_marker_size_2d(style, 3.2),
                            style.color,
                            style.alpha);
      }
    }
  }
};

class ScatterArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), trace.y.size());
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    const std::string marker = detail::canonical_marker_name(style.marker, true);
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y[i]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      detail::draw_marker(surface, px, py, marker, effective_marker_size_2d(style, 3.4), style.color, style.alpha);
    }
  }
};

class StepArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    const auto points = make_step_points_2d(trace, transform);
    surface.stroke_polyline(points,
                            StrokeStyle2D{
                                .color = style.color,
                                .line_width = style.line_width,
                                .alpha = style.alpha,
                                .line_style = style.line_style,
                            });
    if (detail::canonical_marker_name(style.marker, false) != "none") {
      const std::size_t n = std::min(trace.x.size(), trace.y.size());
      for (std::size_t i = 0; i < n; ++i) {
        const double px = transform.sx(trace.x[i]);
        const double py = transform.sy(trace.y[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        detail::draw_marker(surface,
                            px,
                            py,
                            style.marker,
                            effective_marker_size_2d(style, 3.0),
                            style.color,
                            style.alpha);
      }
    }
  }
};

class BarArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), trace.y.size());
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    const bool linear_y = transform.y_axis && transform.y_axis->scale == AxisScale::Linear;
    constexpr double bar_fill_fraction = 0.78;
    const bool explicit_widths = trace.x2.size() == trace.x.size();

    for (std::size_t i = 0; i < n; ++i) {
      const auto [x_lo, x_hi] = bar_bounds_2d(trace, i);
      const double px_lo = transform.sx(x_lo);
      const double px_hi = transform.sx(x_hi);
      if (!std::isfinite(px_lo) || !std::isfinite(px_hi)) {
        continue;
      }
      const double py = transform.sy(trace.y[i]);
      const double baseline_value = (trace.y2.size() == trace.y.size()) ? trace.y2[i] : 0.0;
      const double baseline_screen = linear_y ? transform.sy(baseline_value) : transform.bottom;
      if (!std::isfinite(py) || !std::isfinite(baseline_screen)) {
        continue;
      }
      const double slot_left = std::min(px_lo, px_hi);
      const double slot_width = std::abs(px_hi - px_lo);
      const double inset = explicit_widths ? 0.0 : slot_width * (1.0 - bar_fill_fraction) * 0.5;
      const double rect_x = slot_left + inset;
      const double barw = explicit_widths ? std::max(2.0, slot_width) : std::max(2.0, slot_width - 2.0 * inset);
      const double ytop = std::min(py, baseline_screen);
      const double h = std::max(1.0, std::abs(baseline_screen - py));
      surface.fill_rect(rect_x, ytop, barw, h, FillStyle2D{.color = style.color, .alpha = style.alpha}, 2.0);
      surface.stroke_rect(rect_x, ytop, barw, h,
                          StrokeStyle2D{.color = "white", .line_width = 1.0, .alpha = 0.8, .line_style = "solid"},
                          2.0);
    }
  }
};

class AreaArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), trace.y.size());
    if (n == 0) {
      return;
    }
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    std::vector<Point2D> polygon;
    polygon.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y[i]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      polygon.push_back({px, py});
    }
    for (std::size_t i = n; i > 0; --i) {
      const std::size_t idx = i - 1;
      const double px = transform.sx(trace.x[idx]);
      const double base = (idx < trace.y2.size()) ? trace.y2[idx] : 0.0;
      const double py = transform.sy(base);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      polygon.push_back({px, py});
    }
    surface.fill_polygon(polygon, FillStyle2D{.color = style.color, .alpha = default_fill_alpha_2d(trace)});

    std::vector<Point2D> line;
    line.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y[i]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      line.push_back({px, py});
    }
    surface.stroke_polyline(line,
                            StrokeStyle2D{
                                .color = style.color,
                                .line_width = std::max(1.2, style.line_width),
                                .alpha = 0.95,
                                .line_style = style.line_style,
                            });
  }
};

class BandArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.y2.size()));
    if (n == 0) {
      return;
    }
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    std::vector<Point2D> polygon;
    polygon.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y2[i]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      polygon.push_back({px, py});
    }
    for (std::size_t i = n; i > 0; --i) {
      const std::size_t idx = i - 1;
      const double px = transform.sx(trace.x[idx]);
      const double py = transform.sy(trace.y[idx]);
      if (!std::isfinite(px) || !std::isfinite(py)) {
        continue;
      }
      polygon.push_back({px, py});
    }
    surface.fill_polygon(polygon, FillStyle2D{.color = style.color, .alpha = default_fill_alpha_2d(trace)});

    std::vector<Point2D> lower;
    std::vector<Point2D> upper;
    lower.reserve(n);
    upper.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py_lo = transform.sy(trace.y[i]);
      const double py_hi = transform.sy(trace.y2[i]);
      if (std::isfinite(px) && std::isfinite(py_lo)) {
        lower.push_back({px, py_lo});
      }
      if (std::isfinite(px) && std::isfinite(py_hi)) {
        upper.push_back({px, py_hi});
      }
    }
    surface.stroke_polyline(lower,
                            StrokeStyle2D{
                                .color = style.color,
                                .line_width = std::max(1.0, style.line_width * 0.8),
                                .alpha = 0.85,
                                .line_style = style.line_style,
                            });
    surface.stroke_polyline(upper,
                            StrokeStyle2D{
                                .color = style.color,
                                .line_width = std::max(1.0, style.line_width * 0.8),
                                .alpha = 0.85,
                                .line_style = style.line_style,
                            });
  }
};

class HeatmapArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    if (trace.rows <= 0 || trace.cols <= 0 ||
        trace.values.size() != static_cast<std::size_t>(trace.rows * trace.cols)) {
      return;
    }
    const auto [vmin, vmax] = finite_minmax_2d(trace.values);
    for (std::int64_t row = 0; row < trace.rows; ++row) {
      for (std::int64_t col = 0; col < trace.cols; ++col) {
        const std::size_t idx = static_cast<std::size_t>(row * trace.cols + col);
        const auto [x_lo, x_hi] = cell_bounds_2d(trace.x, static_cast<std::size_t>(col));
        const auto [y_lo, y_hi] = cell_bounds_2d(trace.y2, static_cast<std::size_t>(row));
        const double px1 = transform.sx(x_lo);
        const double px2 = transform.sx(x_hi);
        const double py1 = transform.sy(y_lo);
        const double py2 = transform.sy(y_hi);
        if (!std::isfinite(px1) || !std::isfinite(px2) || !std::isfinite(py1) || !std::isfinite(py2)) {
          continue;
        }
        const double left = std::min(px1, px2);
        const double top = std::min(py1, py2);
        const double width = std::abs(px2 - px1);
        const double height = std::abs(py2 - py1);
        const std::string color = heatmap_color_2d(trace.values[idx], vmin, vmax);
        surface.fill_rect(left, top, width, height, FillStyle2D{.color = color, .alpha = 1.0});
        if (trace.rows * trace.cols <= 144) {
          surface.stroke_rect(left, top, width, height,
                              StrokeStyle2D{.color = "#f8fafc", .line_width = 0.8, .alpha = 0.85, .line_style = "solid"});
        }
        if (trace.rows * trace.cols <= 36 && std::isfinite(trace.values[idx])) {
          surface.text(left + width * 0.5, top + height * 0.55, detail::format_double(trace.values[idx], 3),
                       TextStyle2D{
                           .color = (trace.values[idx] > (vmin + vmax) * 0.5) ? "#111827" : "#f8fafc",
                           .size = 9.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Center,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = trace.style.font_family,
                       });
        }
      }
    }
  }
};

class AnnotationArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    if (trace.x.empty() || trace.y.empty() || trace.text.empty()) {
      return;
    }
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    const double anchor_x = transform.sx(trace.x.front());
    const double anchor_y = transform.sy(trace.y.front());
    if (!std::isfinite(anchor_x) || !std::isfinite(anchor_y)) {
      return;
    }
    const bool explicit_label_position = !trace.x2.empty() && !trace.y2.empty();
    const double default_dx = anchor_x > transform.left + transform.plot_width() * 0.72 ? -18.0 : 18.0;
    const double default_dy = anchor_y < transform.top + transform.plot_height() * 0.2 ? 16.0 : -14.0;
    const double unclamped_x = explicit_label_position ? transform.sx(trace.x2.front()) : anchor_x + default_dx;
    const double unclamped_y = explicit_label_position ? transform.sy(trace.y2.front()) : anchor_y + default_dy;
    const double label_x = std::clamp(unclamped_x, transform.left + 8.0, transform.right - 8.0);
    const double label_y = std::clamp(unclamped_y, transform.top + 12.0, transform.bottom - 8.0);
    const TextAnchor2D text_anchor = explicit_label_position ? TextAnchor2D::Left
                                                             : (default_dx < 0.0 ? TextAnchor2D::Right : TextAnchor2D::Left);
    if (std::isfinite(label_x) && std::isfinite(label_y) &&
        (std::abs(label_x - anchor_x) > 2.0 || std::abs(label_y - anchor_y) > 2.0)) {
      const double line_start_x = (text_anchor == TextAnchor2D::Right) ? label_x - 2.0 : label_x + 2.0;
      const double line_start_y = label_y + (label_y > anchor_y ? 2.0 : -4.0);
      surface.stroke_line(line_start_x, line_start_y, anchor_x, anchor_y,
                          StrokeStyle2D{
                              .color = style.color,
                              .line_width = 1.0,
                              .alpha = 0.9,
                              .line_style = "solid",
                          });
    }
    detail::draw_marker(surface, anchor_x, anchor_y, "circle", 2.5, style.color, 1.0);
    surface.text(label_x, label_y, trace.text,
                 TextStyle2D{
                     .color = style.color,
                     .size = 11.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = text_anchor,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = trace.style.font_family,
                 });
  }
};

class ErrorBarArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.y2.size()));
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      const double py = transform.sy(trace.y[i]);
      const double py_lo = transform.sy(trace.y[i] - trace.y2[i]);
      const double py_hi = transform.sy(trace.y[i] + trace.y2[i]);
      if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(py_lo) || !std::isfinite(py_hi)) {
        continue;
      }
      surface.stroke_line(px, py_lo, px, py_hi,
                          StrokeStyle2D{
                              .color = style.color,
                              .line_width = std::max(1.0, style.line_width),
                              .alpha = style.alpha,
                              .line_style = "solid",
                          });
      surface.stroke_line(px - 4.0, py_lo, px + 4.0, py_lo,
                          StrokeStyle2D{
                              .color = style.color,
                              .line_width = std::max(1.0, style.line_width),
                              .alpha = style.alpha,
                              .line_style = "solid",
                          });
      surface.stroke_line(px - 4.0, py_hi, px + 4.0, py_hi,
                          StrokeStyle2D{
                              .color = style.color,
                              .line_width = std::max(1.0, style.line_width),
                              .alpha = style.alpha,
                              .line_style = "solid",
                          });
      detail::draw_marker(surface,
                          px,
                          py,
                          style.marker == "none" ? "circle" : style.marker,
                          effective_marker_size_2d(style, 2.8),
                          style.color,
                          style.alpha);
    }
  }
};

inline const Artist2D& artist_for_trace_2d(Trace2DKind kind) {
  static const LineArtist2D line_artist{};
  static const ScatterArtist2D scatter_artist{};
  static const StepArtist2D step_artist{};
  static const BarArtist2D bar_artist{};
  static const AreaArtist2D area_artist{};
  static const BandArtist2D band_artist{};
  static const HeatmapArtist2D heatmap_artist{};
  static const AnnotationArtist2D annotation_artist{};
  static const ErrorBarArtist2D errorbar_artist{};
  switch (kind) {
    case Trace2DKind::Line:
      return line_artist;
    case Trace2DKind::Scatter:
      return scatter_artist;
    case Trace2DKind::Step:
      return step_artist;
    case Trace2DKind::Bar:
    case Trace2DKind::Hist:
      return bar_artist;
    case Trace2DKind::Area:
      return area_artist;
    case Trace2DKind::Band:
      return band_artist;
    case Trace2DKind::Heatmap:
      return heatmap_artist;
    case Trace2DKind::Annotation:
      return annotation_artist;
    case Trace2DKind::ErrorBar:
      return errorbar_artist;
    default:
      return line_artist;
  }
}

} // namespace pluss::plotism
