#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/projector_3d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

struct PlotTransform3D {
  double xmin{0.0};
  double xmax{1.0};
  double ymin{0.0};
  double ymax{1.0};
  double zmin{0.0};
  double zmax{1.0};
  double left{0.0};
  double top{0.0};
  double width{1.0};
  double height{1.0};
  const XAxis* x_axis{nullptr};
  const YAxis* y_axis{nullptr};
  const ZAxis* z_axis{nullptr};
  const Camera3D* camera{nullptr};

  detail::ProjectedPoint3D project_normalized(double xn, double yn, double zn) const {
    if (camera == nullptr) {
      return {};
    }
    return detail::project_normalized_3d(xn, yn, zn, *camera, left, top, width, height);
  }

  detail::ProjectedPoint3D project(double x, double y, double z) const {
    if (x_axis == nullptr || y_axis == nullptr || z_axis == nullptr || camera == nullptr) {
      return {};
    }
    return detail::project_value_3d(
        x, y, z, *x_axis, *y_axis, *z_axis, xmin, xmax, ymin, ymax, zmin, zmax, *camera, left, top, width, height);
  }
};

inline std::pair<double, double> finite_minmax_3d_trace(const std::vector<double>& values) {
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

class Artist3D {
public:
  virtual ~Artist3D() = default;
  virtual void draw(RenderSurface2D& surface,
                    const Trace3D& trace,
                    const PlotTransform3D& transform,
                    const ResolvedTheme& theme) const = 0;
};

class LineArtist3D final : public Artist3D {
public:
  void draw(RenderSurface2D& surface,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme&) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    std::vector<Point2D> points;
    points.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      const auto p = transform.project(trace.x[i], trace.y[i], trace.z[i]);
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        continue;
      }
      points.push_back({p.x, p.y});
    }
    surface.stroke_polyline(points,
                            StrokeStyle2D{
                                .color = trace.style.color,
                                .line_width = trace.style.line_width,
                                .alpha = trace.style.alpha,
                                .line_style = trace.style.line_style,
                            });
    if (detail::canonical_marker_name(trace.style.marker, false) != "none") {
      for (const auto& point : points) {
        detail::draw_marker(surface, point.x, point.y, trace.style.marker, 3.0, trace.style.color, trace.style.alpha);
      }
    }
  }
};

class ScatterArtist3D final : public Artist3D {
public:
  void draw(RenderSurface2D& surface,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme&) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    const std::string marker = detail::canonical_marker_name(trace.style.marker, true);
    for (std::size_t i = 0; i < n; ++i) {
      const auto p = transform.project(trace.x[i], trace.y[i], trace.z[i]);
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        continue;
      }
      detail::draw_marker(surface, p.x, p.y, marker, 3.0, trace.style.color, trace.style.alpha);
    }
  }
};

class SurfaceArtist3D final : public Artist3D {
public:
  void draw(RenderSurface2D& surface,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme& theme) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    if (trace.rows <= 1 || trace.cols <= 1 || static_cast<std::size_t>(trace.rows * trace.cols) != n) {
      return;
    }

    struct SurfaceCell3D {
      std::vector<Point2D> polygon{};
      double avg_depth{0.0};
      double avg_z{0.0};
    };

    const auto [trace_zmin, trace_zmax] = finite_minmax_3d_trace(trace.z);
    std::vector<SurfaceCell3D> cells;
    cells.reserve(static_cast<std::size_t>((trace.rows - 1) * (trace.cols - 1)));
    for (std::int64_t row = 0; row + 1 < trace.rows; ++row) {
      for (std::int64_t col = 0; col + 1 < trace.cols; ++col) {
        const std::size_t i00 = static_cast<std::size_t>(row * trace.cols + col);
        const std::size_t i01 = static_cast<std::size_t>(row * trace.cols + (col + 1));
        const std::size_t i10 = static_cast<std::size_t>((row + 1) * trace.cols + col);
        const std::size_t i11 = static_cast<std::size_t>((row + 1) * trace.cols + (col + 1));
        const std::array<detail::ProjectedPoint3D, 4> quad{{
            transform.project(trace.x[i00], trace.y[i00], trace.z[i00]),
            transform.project(trace.x[i01], trace.y[i01], trace.z[i01]),
            transform.project(trace.x[i11], trace.y[i11], trace.z[i11]),
            transform.project(trace.x[i10], trace.y[i10], trace.z[i10]),
        }};

        bool valid = true;
        std::vector<Point2D> polygon;
        polygon.reserve(4);
        double depth_sum = 0.0;
        for (const auto& p : quad) {
          if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.depth)) {
            valid = false;
            break;
          }
          polygon.push_back({p.x, p.y});
          depth_sum += p.depth;
        }
        if (!valid) {
          continue;
        }
        cells.push_back(SurfaceCell3D{
            .polygon = std::move(polygon),
            .avg_depth = depth_sum * 0.25,
            .avg_z = (trace.z[i00] + trace.z[i01] + trace.z[i10] + trace.z[i11]) * 0.25,
        });
      }
    }

    std::sort(cells.begin(), cells.end(), [](const SurfaceCell3D& a, const SurfaceCell3D& b) {
      return a.avg_depth < b.avg_depth;
    });

    for (const auto& cell : cells) {
      const std::string fill = detail::surface_color_3d(cell.avg_z, trace_zmin, trace_zmax);
      surface.fill_polygon(cell.polygon, FillStyle2D{.color = fill, .alpha = std::min(1.0, trace.style.alpha)});
      std::vector<Point2D> outline{cell.polygon[0], cell.polygon[1], cell.polygon[2], cell.polygon[3], cell.polygon[0]};
      surface.stroke_polyline(outline,
                              StrokeStyle2D{
                                  .color = theme.plot_border,
                                  .line_width = 0.8,
                                  .alpha = 0.45,
                                  .line_style = "solid",
                              });
    }
  }
};

class LegendArtist3D {
public:
  void draw_sample(RenderSurface2D& surface,
                   const Trace3D& trace,
                   double x1,
                   double x2,
                   double y,
                   const ResolvedTheme& theme) const {
    if (trace.kind == Trace3DKind::Surface) {
      surface.fill_rect(x1, y - 4.0, x2 - x1, 8.0,
                        FillStyle2D{.color = detail::surface_color_3d(0.75, 0.0, 1.0), .alpha = 0.95},
                        2.0);
      surface.stroke_rect(x1, y - 4.0, x2 - x1, 8.0,
                          StrokeStyle2D{
                              .color = theme.plot_border,
                              .line_width = 0.8,
                              .alpha = 0.8,
                              .line_style = "solid",
                          },
                          2.0);
      return;
    }

    surface.stroke_line(x1, y, x2, y,
                        StrokeStyle2D{
                            .color = trace.style.color,
                            .line_width = 2.0,
                            .alpha = 1.0,
                            .line_style = trace.style.line_style,
                        });
    const std::string marker = (trace.kind == Trace3DKind::Line3) ? detail::canonical_marker_name(trace.style.marker, false)
                                                                  : detail::canonical_marker_name(trace.style.marker, true);
    if (marker != "none") {
      detail::draw_marker(surface, (x1 + x2) * 0.5, y, marker, 2.7, trace.style.color, trace.style.alpha);
    }
  }
};

inline const Artist3D& artist_for_trace_3d(Trace3DKind kind) {
  static const LineArtist3D line_artist{};
  static const ScatterArtist3D scatter_artist{};
  static const SurfaceArtist3D surface_artist{};
  switch (kind) {
    case Trace3DKind::Line3:
      return line_artist;
    case Trace3DKind::Scatter3:
      return scatter_artist;
    case Trace3DKind::Surface:
    default:
      return surface_artist;
  }
}

} // namespace pluss::plotism
