#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plot/modules/axes.h"
#include "plot/modules/core.h"
#include "plot/modules/style.h"

namespace pluss::plotism {

struct Point2D {
  double x{0.0};
  double y{0.0};
};

enum class TextAnchor2D : std::uint8_t {
  Left = 0,
  Center = 1,
  Right = 2,
};

enum class TextOrientation2D : std::uint8_t {
  Horizontal = 0,
  VerticalCcw = 1,
};

struct FillStyle2D {
  std::string color{"black"};
  double alpha{1.0};
};

struct StrokeStyle2D {
  std::string color{"black"};
  double line_width{1.0};
  double alpha{1.0};
  std::string line_style{"solid"};
};

struct TextStyle2D {
  std::string color{"black"};
  double size{12.0};
  double alpha{1.0};
  bool bold{false};
  TextAnchor2D anchor{TextAnchor2D::Left};
  TextOrientation2D orientation{TextOrientation2D::Horizontal};
};

class RenderSurface2D {
public:
  virtual ~RenderSurface2D() = default;

  virtual void fill_rect(double x, double y, double w, double h, const FillStyle2D& style, double corner_radius = 0.0) = 0;
  virtual void stroke_rect(double x,
                           double y,
                           double w,
                           double h,
                           const StrokeStyle2D& style,
                           double corner_radius = 0.0) = 0;
  virtual void stroke_line(double x1, double y1, double x2, double y2, const StrokeStyle2D& style) = 0;
  virtual void stroke_polyline(const std::vector<Point2D>& points, const StrokeStyle2D& style) = 0;
  virtual void fill_circle(double cx, double cy, double r, const FillStyle2D& style) = 0;
  virtual void fill_polygon(const std::vector<Point2D>& points, const FillStyle2D& style) = 0;
  virtual void text(double x, double y, std::string_view text, const TextStyle2D& style) = 0;
  virtual std::string finish() const = 0;
};

namespace detail {

inline double axis_transform_2d(double value, const Axis& axis) {
  return axis.transform_value(value);
}

inline std::pair<double, double> ensure_limits_2d(double lo, double hi) {
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    return {0.0, 1.0};
  }
  if (lo == hi) {
    return {lo - 0.5, hi + 0.5};
  }
  if (lo > hi) {
    std::swap(lo, hi);
  }
  return {lo, hi};
}

inline std::pair<double, double> map_legend_anchor_2d(LegendLocation location,
                                                       double left,
                                                       double right,
                                                       double top,
                                                       double bottom,
                                                       double legend_width,
                                                       double legend_height) {
  const double margin = 6.0;
  switch (location) {
    case LegendLocation::TopLeft:
      return {left + margin, top + margin};
    case LegendLocation::BottomLeft:
      return {left + margin, bottom - legend_height - margin};
    case LegendLocation::BottomRight:
      return {right - legend_width - margin, bottom - legend_height - margin};
    case LegendLocation::Best:
    case LegendLocation::TopRight:
    default:
      return {right - legend_width - margin, top + margin};
  }
}

inline double nice_tick_step_2d(double lo, double hi, int approx_tick_count = 5) {
  const double span = std::abs(hi - lo);
  if (!(span > 0.0) || !std::isfinite(span)) {
    return 1.0;
  }
  const double raw = span / static_cast<double>(std::max(1, approx_tick_count));
  const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
  const double residual = raw / magnitude;
  double nice = 1.0;
  if (residual >= 5.0) {
    nice = 5.0;
  } else if (residual >= 2.0) {
    nice = 2.0;
  }
  return nice * magnitude;
}

inline std::vector<double> make_ticks_2d(double lo, double hi, int approx_tick_count = 5) {
  std::vector<double> ticks;
  const auto [min_v, max_v] = ensure_limits_2d(lo, hi);
  const double step = nice_tick_step_2d(min_v, max_v, approx_tick_count);
  const double start = std::ceil(min_v / step) * step;
  for (double value = start; value <= max_v + step * 0.5; value += step) {
    if (std::isfinite(value)) {
      ticks.push_back(std::abs(value) < step * 1e-9 ? 0.0 : value);
    }
  }
  if (ticks.empty()) {
    ticks.push_back(min_v);
    ticks.push_back(max_v);
  }
  return ticks;
}

inline std::string format_tick_label_2d(double transformed_value, const Axis& axis) {
  const auto ticks = axis.resolve_ticks(transformed_value, transformed_value);
  return ticks.empty() ? format_double(transformed_value, 6) : ticks.front().label;
}

inline std::vector<ResolvedTick> resolve_ticks_2d(const Axis& axis,
                                                  double lo,
                                                  double hi,
                                                  const std::vector<double>* categorical_positions = nullptr,
                                                  const std::vector<std::string>* categorical_labels = nullptr) {
  return axis.resolve_ticks(lo, hi, categorical_positions, categorical_labels);
}

inline double legend_width_hint_2d(std::size_t entry_count, std::size_t longest_label, bool has_title) {
  const double label_width = 42.0 + static_cast<double>(longest_label) * 6.2;
  const double title_extra = has_title ? 16.0 : 0.0;
  return std::max(110.0, std::min(220.0, label_width + title_extra + static_cast<double>(entry_count > 9 ? 10 : 0)));
}

inline double legend_height_hint_2d(std::size_t entry_count, bool has_title) {
  return 8.0 + static_cast<double>(entry_count) * 16.0 + (has_title ? 18.0 : 0.0);
}

inline std::string canonical_marker_name(std::string_view marker, bool scatter_like) {
  if (marker.empty() || marker == "none") {
    return scatter_like ? "circle" : "none";
  }
  if (marker == "o") return "circle";
  if (marker == "s") return "square";
  if (marker == "^") return "triangle";
  if (marker == "d") return "diamond";
  if (marker == "x") return "x";
  if (marker == "+") return "plus";
  return std::string(marker);
}

inline void draw_marker(RenderSurface2D& surface,
                        double x,
                        double y,
                        std::string_view marker,
                        double size,
                        std::string_view color,
                        double alpha = 1.0) {
  const std::string kind = canonical_marker_name(marker, true);
  if (kind == "none") {
    return;
  }

  const double half = std::max(2.0, size);
  const FillStyle2D fill{.color = std::string(color), .alpha = alpha};
  const StrokeStyle2D stroke{
      .color = std::string(color),
      .line_width = 1.4,
      .alpha = alpha,
      .line_style = "solid",
  };

  if (kind == "circle") {
    surface.fill_circle(x, y, half, fill);
    return;
  }
  if (kind == "square") {
    surface.fill_rect(x - half, y - half, half * 2.0, half * 2.0, fill);
    return;
  }
  if (kind == "triangle") {
    surface.fill_polygon({{x, y - half}, {x + half, y + half}, {x - half, y + half}}, fill);
    return;
  }
  if (kind == "diamond") {
    surface.fill_polygon({{x, y - half}, {x + half, y}, {x, y + half}, {x - half, y}}, fill);
    return;
  }
  if (kind == "x") {
    surface.stroke_line(x - half, y - half, x + half, y + half, stroke);
    surface.stroke_line(x - half, y + half, x + half, y - half, stroke);
    return;
  }
  if (kind == "plus") {
    surface.stroke_line(x - half, y, x + half, y, stroke);
    surface.stroke_line(x, y - half, x, y + half, stroke);
  }
}

} // namespace detail
} // namespace pluss::plotism
