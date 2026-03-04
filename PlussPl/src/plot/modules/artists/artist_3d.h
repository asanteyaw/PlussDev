#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/projector_3d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

inline double effective_marker_size_3d(const Style& style, double fallback) {
  return style.marker_size > 0.0 ? style.marker_size : fallback;
}

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
  double projection_extent{0.88};
  std::array<double, 3> box_aspect{1.0, 1.0, 1.0};
  const XAxis* x_axis{nullptr};
  const YAxis* y_axis{nullptr};
  const ZAxis* z_axis{nullptr};
  const Camera3D* camera{nullptr};

  detail::ProjectedPoint3D project_normalized(double xn, double yn, double zn) const {
    if (camera == nullptr) {
      return {};
    }
    return detail::project_normalized_3d(xn, yn, zn, *camera, box_aspect, projection_extent, left, top, width, height);
  }

  detail::ProjectedPoint3D project(double x, double y, double z) const {
    if (x_axis == nullptr || y_axis == nullptr || z_axis == nullptr || camera == nullptr) {
      return {};
    }
    return detail::project_value_3d(
        x, y, z, *x_axis, *y_axis, *z_axis, xmin, xmax, ymin, ymax, zmin, zmax, *camera, box_aspect, projection_extent, left, top, width, height);
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

struct ScenePrimitive3D {
  double depth{0.0};
  int layer{0};
  std::function<void(RenderSurface2D&)> paint{};
};

class DepthSorter3D {
public:
  void add(ScenePrimitive3D primitive) { primitives_.push_back(std::move(primitive)); }

  void paint(RenderSurface2D& surface) {
    std::stable_sort(primitives_.begin(), primitives_.end(), [](const ScenePrimitive3D& lhs, const ScenePrimitive3D& rhs) {
      if (lhs.depth == rhs.depth) {
        return lhs.layer < rhs.layer;
      }
      return lhs.depth < rhs.depth;
    });
    for (auto& primitive : primitives_) {
      primitive.paint(surface);
    }
  }

private:
  std::vector<ScenePrimitive3D> primitives_{};
};

inline double signed_area_2d(const std::vector<Point2D>& polygon) {
  if (polygon.size() < 3) {
    return 0.0;
  }
  double twice_area = 0.0;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const auto& a = polygon[i];
    const auto& b = polygon[(i + 1) % polygon.size()];
    twice_area += a.x * b.y - b.x * a.y;
  }
  return 0.5 * twice_area;
}

inline void emit_surface_segment(DepthSorter3D& sorter,
                                 const detail::ProjectedPoint3D& p0,
                                 const detail::ProjectedPoint3D& p1,
                                 const std::string& color,
                                 double line_width,
                                 double alpha,
                                 std::string line_style,
                                 int layer) {
  if (!std::isfinite(p0.x) || !std::isfinite(p0.y) || !std::isfinite(p0.depth) ||
      !std::isfinite(p1.x) || !std::isfinite(p1.y) || !std::isfinite(p1.depth)) {
    return;
  }
  sorter.add(ScenePrimitive3D{
      .depth = 0.5 * (p0.depth + p1.depth),
      .layer = layer,
      .paint =
          [p0, p1, color, line_width, alpha, line_style = std::move(line_style)](RenderSurface2D& surface) {
            surface.stroke_line(p0.x, p0.y, p1.x, p1.y,
                                StrokeStyle2D{
                                    .color = color,
                                    .line_width = line_width,
                                    .alpha = alpha,
                                    .line_style = line_style,
                                });
          },
  });
}

class Artist3D {
public:
  virtual ~Artist3D() = default;
  virtual void emit(DepthSorter3D& sorter,
                    const Trace3D& trace,
                    const PlotTransform3D& transform,
                    const ResolvedTheme& theme) const = 0;
};

class LineArtist3D final : public Artist3D {
public:
  void emit(DepthSorter3D& sorter,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme&) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    std::vector<detail::ProjectedPoint3D> points;
    points.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      const auto projected = transform.project(trace.x[i], trace.y[i], trace.z[i]);
      if (!std::isfinite(projected.x) || !std::isfinite(projected.y) || !std::isfinite(projected.depth)) {
        continue;
      }
      points.push_back(projected);
    }
    for (std::size_t i = 1; i < points.size(); ++i) {
      const auto p0 = points[i - 1];
      const auto p1 = points[i];
      sorter.add(ScenePrimitive3D{
          .depth = 0.5 * (p0.depth + p1.depth),
          .layer = 2,
          .paint =
              [p0, p1, style = trace.style](RenderSurface2D& surface) {
                surface.stroke_line(p0.x, p0.y, p1.x, p1.y,
                                    StrokeStyle2D{
                                        .color = style.color,
                                        .line_width = style.line_width,
                                        .alpha = style.alpha,
                                        .line_style = style.line_style,
                                    });
              },
      });
    }
    if (detail::canonical_marker_name(trace.style.marker, false) != "none") {
      for (const auto& point : points) {
        sorter.add(ScenePrimitive3D{
            .depth = point.depth,
            .layer = 3,
            .paint =
                [point, style = trace.style](RenderSurface2D& surface) {
                  detail::draw_marker(surface,
                                      point.x,
                                      point.y,
                                      style.marker,
                                      effective_marker_size_3d(style, 3.0),
                                      style.color,
                                      style.alpha);
                },
        });
      }
    }
  }
};

class ScatterArtist3D final : public Artist3D {
public:
  void emit(DepthSorter3D& sorter,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme&) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    const std::string marker = detail::canonical_marker_name(trace.style.marker, true);
    for (std::size_t i = 0; i < n; ++i) {
      const auto p = transform.project(trace.x[i], trace.y[i], trace.z[i]);
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.depth)) {
        continue;
      }
      sorter.add(ScenePrimitive3D{
          .depth = p.depth,
          .layer = 3,
          .paint =
              [p, marker, style = trace.style](RenderSurface2D& surface) {
                detail::draw_marker(surface,
                                    p.x,
                                    p.y,
                                    marker,
                                    effective_marker_size_3d(style, 3.0),
                                    style.color,
                                    style.alpha);
              },
      });
    }
  }
};

class SurfaceArtist3D final : public Artist3D {
public:
  void emit(DepthSorter3D& sorter,
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
      bool front_facing{true};
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
            .front_facing = signed_area_2d(polygon) <= 0.0,
        });
      }
    }

    for (const auto& cell : cells) {
      const std::string fill = detail::surface_color_3d(cell.avg_z, trace_zmin, trace_zmax);
      sorter.add(ScenePrimitive3D{
          .depth = cell.avg_depth,
          .layer = 0,
          .paint =
              [polygon = cell.polygon, fill, alpha = std::min(1.0, trace.style.alpha)](RenderSurface2D& surface) {
                surface.fill_polygon(polygon, FillStyle2D{.color = fill, .alpha = alpha});
              },
      });

      if (trace.wireframe) {
        std::vector<Point2D> outline{cell.polygon[0], cell.polygon[1], cell.polygon[2], cell.polygon[3], cell.polygon[0]};
        sorter.add(ScenePrimitive3D{
            .depth = cell.avg_depth,
            .layer = 1,
            .paint =
                [outline = std::move(outline), front_facing = cell.front_facing, border = theme.plot_border](RenderSurface2D& surface) {
                  surface.stroke_polyline(outline,
                                          StrokeStyle2D{
                                              .color = border,
                                              .line_width = front_facing ? 0.85 : 0.55,
                                              .alpha = front_facing ? 0.42 : 0.10,
                                              .line_style = front_facing ? "solid" : "dashed",
                                          });
                },
        });
      }
    }

    if (!trace.contours) {
      return;
    }

    const std::string contour_color = trace.style.color.empty() ? theme.axis_color : trace.style.color;
    const std::int32_t row_stride = std::max<std::int32_t>(1, trace.contour_stride);
    const std::int32_t col_stride = std::max<std::int32_t>(1, trace.contour_stride);
    for (std::int64_t row = 0; row < trace.rows; row += row_stride) {
      for (std::int64_t col = 1; col < trace.cols; ++col) {
        const std::size_t i0 = static_cast<std::size_t>(row * trace.cols + (col - 1));
        const std::size_t i1 = static_cast<std::size_t>(row * trace.cols + col);
        emit_surface_segment(sorter,
                             transform.project(trace.x[i0], trace.y[i0], trace.z[i0]),
                             transform.project(trace.x[i1], trace.y[i1], trace.z[i1]),
                             contour_color,
                             1.35,
                             0.85,
                             "solid",
                             2);
      }
    }
    for (std::int64_t col = 0; col < trace.cols; col += col_stride) {
      for (std::int64_t row = 1; row < trace.rows; ++row) {
        const std::size_t i0 = static_cast<std::size_t>((row - 1) * trace.cols + col);
        const std::size_t i1 = static_cast<std::size_t>(row * trace.cols + col);
        emit_surface_segment(sorter,
                             transform.project(trace.x[i0], trace.y[i0], trace.z[i0]),
                             transform.project(trace.x[i1], trace.y[i1], trace.z[i1]),
                             contour_color,
                             1.35,
                             0.85,
                             "solid",
                             2);
      }
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
      detail::draw_marker(surface,
                          (x1 + x2) * 0.5,
                          y,
                          marker,
                          effective_marker_size_3d(trace.style, 2.7),
                          trace.style.color,
                          trace.style.alpha);
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
