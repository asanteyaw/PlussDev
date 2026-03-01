#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"
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
};

inline ArtistStyle2D resolve_artist_style_2d(const Trace2D& trace, const std::string& fallback_color) {
  return ArtistStyle2D{
      .color = trace.style.color.empty() ? fallback_color : trace.style.color,
      .line_width = trace.style.line_width,
      .alpha = trace.style.alpha,
      .line_style = trace.style.line_style,
      .marker = trace.style.marker,
  };
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
      detail::draw_marker(surface, (x1 + x2) * 0.5, y, marker, 2.7, color, trace.style.alpha);
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
        detail::draw_marker(surface, point.x, point.y, style.marker, 3.2, style.color, style.alpha);
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
      detail::draw_marker(surface, px, py, marker, 3.4, style.color, style.alpha);
    }
  }
};

class BarArtist2D final : public Artist2D {
public:
  void draw(RenderSurface2D& surface, const Trace2D& trace, const PlotTransform2D& transform) const override {
    const std::size_t n = std::min(trace.x.size(), trace.y.size());
    const auto style = resolve_artist_style_2d(trace, "#1f77b4");
    const double barw = std::max(2.0, transform.plot_width() / std::max<std::size_t>(1, trace.x.size()) * 0.72);
    const bool linear_y = transform.y_axis && transform.y_axis->scale == AxisScale::Linear;
    const double baseline_val = linear_y ? 0.0 : 0.0;
    const double baseline_screen = linear_y ? transform.sy(baseline_val) : transform.bottom;

    for (std::size_t i = 0; i < n; ++i) {
      const double px = transform.sx(trace.x[i]);
      if (!std::isfinite(px)) {
        continue;
      }
      const double py = transform.sy(linear_y ? std::max(baseline_val, trace.y[i]) : trace.y[i]);
      if (!std::isfinite(py) || !std::isfinite(baseline_screen)) {
        continue;
      }
      const double rect_x = px - barw * 0.5;
      const double h = std::abs(baseline_screen - py);
      const double ytop = std::min(py, baseline_screen);
      surface.fill_rect(rect_x, ytop, barw, h, FillStyle2D{.color = style.color, .alpha = style.alpha}, 2.0);
      surface.stroke_rect(rect_x, ytop, barw, h,
                          StrokeStyle2D{.color = "white", .line_width = 1.0, .alpha = 0.8, .line_style = "solid"},
                          2.0);
    }
  }
};

inline const Artist2D& artist_for_trace_2d(Trace2DKind kind) {
  static const LineArtist2D line_artist{};
  static const ScatterArtist2D scatter_artist{};
  static const BarArtist2D bar_artist{};
  switch (kind) {
    case Trace2DKind::Line:
      return line_artist;
    case Trace2DKind::Scatter:
      return scatter_artist;
    case Trace2DKind::Bar:
    case Trace2DKind::Hist:
    default:
      return bar_artist;
  }
}

} // namespace pluss::plotism
