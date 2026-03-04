#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
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

inline std::array<int, 3> hex_to_rgb_3d(std::string color) {
  if (color.size() == 7 && color[0] == '#') {
    const auto hex = [](char ch) -> int {
      if (ch >= '0' && ch <= '9') return ch - '0';
      if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
      if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
      return 0;
    };
    return {
        hex(color[1]) * 16 + hex(color[2]),
        hex(color[3]) * 16 + hex(color[4]),
        hex(color[5]) * 16 + hex(color[6]),
    };
  }
  return {111, 177, 199};
}

inline std::string shade_color_3d(const std::string& color, double intensity) {
  const auto rgb = hex_to_rgb_3d(color);
  const double clamped = std::clamp(intensity, 0.0, 1.45);
  const auto shade = [clamped](int component) {
    if (clamped >= 1.0) {
      return static_cast<int>(std::round(component + (255.0 - component) * std::min(0.45, clamped - 1.0)));
    }
    return static_cast<int>(std::round(component * std::max(0.0, clamped)));
  };
  return detail::hex_color_from_rgb_3d(shade(rgb[0]), shade(rgb[1]), shade(rgb[2]));
}

inline double dot3_3d(const std::array<double, 3>& lhs, const std::array<double, 3>& rhs) {
  return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

inline std::array<double, 3> normalize3_3d(const std::array<double, 3>& v) {
  const double mag = std::sqrt(std::max(0.0, dot3_3d(v, v)));
  if (mag <= 1e-12) {
    return {0.0, 0.0, 1.0};
  }
  return {v[0] / mag, v[1] / mag, v[2] / mag};
}

inline std::array<double, 3> subtract3_3d(const std::array<double, 3>& a, const std::array<double, 3>& b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

inline std::array<double, 3> cross3_3d(const std::array<double, 3>& a, const std::array<double, 3>& b) {
  return {
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0],
  };
}

inline std::array<double, 3> surface_light_direction_3d(double azimuth_deg, double elevation_deg) {
  const double az = azimuth_deg * 3.14159265358979323846 / 180.0;
  const double el = elevation_deg * 3.14159265358979323846 / 180.0;
  return normalize3_3d({
      std::sin(az) * std::cos(el),
      -std::cos(az) * std::cos(el),
      std::sin(el),
  });
}

inline std::array<double, 3> normalized_surface_vertex_3d(double x, double y, double z, const PlotTransform3D& transform) {
  if (transform.x_axis == nullptr || transform.y_axis == nullptr || transform.z_axis == nullptr) {
    return {0.0, 0.0, 0.0};
  }
  const auto normalize_axis = [](double value, const Axis& axis, double lo, double hi) {
    const double transformed = detail::axis_transform_2d(value, axis);
    if (!std::isfinite(transformed) || !(hi > lo)) {
      return 0.0;
    }
    double normalized = (transformed - lo) / (hi - lo);
    if (axis.inverted) {
      normalized = 1.0 - normalized;
    }
    return normalized;
  };
  return {
      normalize_axis(x, *transform.x_axis, transform.xmin, transform.xmax) * transform.box_aspect[0],
      normalize_axis(y, *transform.y_axis, transform.ymin, transform.ymax) * transform.box_aspect[1],
      normalize_axis(z, *transform.z_axis, transform.zmin, transform.zmax) * transform.box_aspect[2],
  };
}

inline double surface_lighting_intensity_3d(const std::array<double, 3>& p00,
                                            const std::array<double, 3>& p01,
                                            const std::array<double, 3>& p10,
                                            const Trace3D& trace,
                                            const PlotTransform3D& transform) {
  if (!trace.lighted || transform.camera == nullptr) {
    return 1.0;
  }
  const std::array<double, 3> normal = normalize3_3d(cross3_3d(subtract3_3d(p01, p00), subtract3_3d(p10, p00)));
  const std::array<double, 3> light = surface_light_direction_3d(trace.light_azimuth_deg, trace.light_elevation_deg);
  const std::array<double, 3> view = normalize3_3d(detail::camera_view_direction_3d(*transform.camera));
  const double diffuse = std::max(0.0, dot3_3d(normal, light));
  const std::array<double, 3> reflected = normalize3_3d({
      2.0 * diffuse * normal[0] - light[0],
      2.0 * diffuse * normal[1] - light[1],
      2.0 * diffuse * normal[2] - light[2],
  });
  const double specular = std::pow(std::max(0.0, dot3_3d(reflected, view)), 10.0);
  constexpr double ambient_weight = 0.55;
  constexpr double diffuse_weight = 0.60;
  constexpr double specular_weight = 0.40;
  return (ambient_weight + diffuse_weight * diffuse + specular_weight * specular) /
         (ambient_weight + diffuse_weight + specular_weight);
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

    struct SurfaceContourSegment3D {
      detail::ProjectedPoint3D p0{};
      detail::ProjectedPoint3D p1{};
      double level{0.0};
      bool valid{false};
    };

    struct SurfaceCell3D {
      std::vector<Point2D> polygon{};
      double avg_depth{0.0};
      double avg_z{0.0};
      bool front_facing{true};
      double lighting{1.0};
    };

    const auto [trace_zmin, trace_zmax] = finite_minmax_3d_trace(trace.z);
    const double base_z = transform.z_axis != nullptr && transform.z_axis->min.has_value() ? *transform.z_axis->min : trace_zmin;
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
        const auto n00 = normalized_surface_vertex_3d(trace.x[i00], trace.y[i00], trace.z[i00], transform);
        const auto n01 = normalized_surface_vertex_3d(trace.x[i01], trace.y[i01], trace.z[i01], transform);
        const auto n10 = normalized_surface_vertex_3d(trace.x[i10], trace.y[i10], trace.z[i10], transform);

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
            .lighting = surface_lighting_intensity_3d(n00, n01, n10, trace, transform),
        });
      }
    }

    for (const auto& cell : cells) {
      if (trace.filled) {
        const std::string base_fill = detail::surface_color_3d(cell.avg_z, trace_zmin, trace_zmax);
        const std::string fill = trace.lighted ? shade_color_3d(base_fill, cell.lighting) : base_fill;
        sorter.add(ScenePrimitive3D{
            .depth = cell.avg_depth,
            .layer = 0,
            .paint =
                [polygon = cell.polygon, fill, alpha = std::min(1.0, trace.style.alpha)](RenderSurface2D& surface) {
                  surface.fill_polygon(polygon, FillStyle2D{.color = fill, .alpha = alpha});
                },
        });
      }

    }

    const auto interpolate_level_point = [&](std::size_t ia, std::size_t ib, double level, double z_target) -> detail::ProjectedPoint3D {
      const double za = trace.z[ia];
      const double zb = trace.z[ib];
      const double denom = zb - za;
      const double t = std::abs(denom) <= 1e-12 ? 0.5 : std::clamp((level - za) / denom, 0.0, 1.0);
      const double x = trace.x[ia] + (trace.x[ib] - trace.x[ia]) * t;
      const double y = trace.y[ia] + (trace.y[ib] - trace.y[ia]) * t;
      return transform.project(x, y, z_target);
    };

    const auto build_contour_segments = [&](double level, double z_target) {
      std::vector<SurfaceContourSegment3D> segments;
      segments.reserve(static_cast<std::size_t>((trace.rows - 1) * (trace.cols - 1)));
      for (std::int64_t row = 0; row + 1 < trace.rows; ++row) {
        for (std::int64_t col = 0; col + 1 < trace.cols; ++col) {
          const std::size_t i00 = static_cast<std::size_t>(row * trace.cols + col);
          const std::size_t i01 = static_cast<std::size_t>(row * trace.cols + (col + 1));
          const std::size_t i10 = static_cast<std::size_t>((row + 1) * trace.cols + col);
          const std::size_t i11 = static_cast<std::size_t>((row + 1) * trace.cols + (col + 1));

          const std::array<std::pair<std::size_t, std::size_t>, 4> edges{{
              {i00, i01},
              {i01, i11},
              {i11, i10},
              {i10, i00},
          }};

          std::vector<detail::ProjectedPoint3D> intersections;
          intersections.reserve(4);
          for (const auto& [ia, ib] : edges) {
            const double za = trace.z[ia];
            const double zb = trace.z[ib];
            const double min_z = std::min(za, zb);
            const double max_z = std::max(za, zb);
            if (!(level >= min_z && level <= max_z) || std::abs(zb - za) <= 1e-12) {
              continue;
            }
            intersections.push_back(interpolate_level_point(ia, ib, level, z_target));
          }

          if (intersections.size() == 2) {
            segments.push_back({.p0 = intersections[0], .p1 = intersections[1], .level = level, .valid = true});
          } else if (intersections.size() == 4) {
            segments.push_back({.p0 = intersections[0], .p1 = intersections[1], .level = level, .valid = true});
            segments.push_back({.p0 = intersections[2], .p1 = intersections[3], .level = level, .valid = true});
          }
        }
      }
      return segments;
    };

    if (trace.wireframe) {
      const auto emit_grid_family = [&](bool along_rows) {
        const std::int64_t outer_count = along_rows ? trace.rows : trace.cols;
        const std::int64_t inner_count = along_rows ? trace.cols : trace.rows;
        for (std::int64_t outer = 0; outer < outer_count; ++outer) {
          for (std::int64_t inner = 1; inner < inner_count; ++inner) {
            const std::size_t i0 = along_rows ? static_cast<std::size_t>(outer * trace.cols + (inner - 1))
                                              : static_cast<std::size_t>((inner - 1) * trace.cols + outer);
            const std::size_t i1 = along_rows ? static_cast<std::size_t>(outer * trace.cols + inner)
                                              : static_cast<std::size_t>(inner * trace.cols + outer);
            const double level = 0.5 * (trace.z[i0] + trace.z[i1]);
            const std::string line_color = trace.filled ? shade_color_3d(detail::surface_color_3d(level, trace_zmin, trace_zmax), 0.9)
                                                        : detail::surface_color_3d(level, trace_zmin, trace_zmax);
            emit_surface_segment(sorter,
                                 transform.project(trace.x[i0], trace.y[i0], trace.z[i0]),
                                 transform.project(trace.x[i1], trace.y[i1], trace.z[i1]),
                                 line_color,
                                 trace.filled ? 0.9 : 1.1,
                                 trace.filled ? 0.5 : 0.88,
                                 "solid",
                                 1);
          }
        }
      };
      if (trace.wireframe_rows) {
        emit_grid_family(true);
      }
      if (trace.wireframe_cols) {
        emit_grid_family(false);
      }
    }

    if (trace.curtain_mode == SurfaceCurtainMode::Boundary) {
      const auto emit_vertical = [&](std::size_t index) {
        emit_surface_segment(sorter,
                             transform.project(trace.x[index], trace.y[index], trace.z[index]),
                             transform.project(trace.x[index], trace.y[index], base_z),
                             theme.plot_border,
                             0.9,
                             0.55,
                             "solid",
                             1);
      };
      const auto emit_base_edge = [&](std::size_t a, std::size_t b) {
        emit_surface_segment(sorter,
                             transform.project(trace.x[a], trace.y[a], base_z),
                             transform.project(trace.x[b], trace.y[b], base_z),
                             theme.plot_border,
                             0.95,
                             0.58,
                             "solid",
                             1);
      };
      for (std::int64_t col = 0; col < trace.cols; ++col) {
        const std::size_t top = static_cast<std::size_t>(col);
        const std::size_t bottom = static_cast<std::size_t>((trace.rows - 1) * trace.cols + col);
        emit_vertical(top);
        emit_vertical(bottom);
        if (col > 0) {
          emit_base_edge(static_cast<std::size_t>(col - 1), top);
          emit_base_edge(static_cast<std::size_t>((trace.rows - 1) * trace.cols + (col - 1)), bottom);
        }
      }
      for (std::int64_t row = 0; row < trace.rows; ++row) {
        const std::size_t left = static_cast<std::size_t>(row * trace.cols);
        const std::size_t right = static_cast<std::size_t>(row * trace.cols + (trace.cols - 1));
        emit_vertical(left);
        emit_vertical(right);
        if (row > 0) {
          emit_base_edge(static_cast<std::size_t>((row - 1) * trace.cols), left);
          emit_base_edge(static_cast<std::size_t>((row - 1) * trace.cols + (trace.cols - 1)), right);
        }
      }
    } else if (trace.curtain_mode == SurfaceCurtainMode::Rows) {
      for (std::int64_t row = 0; row < trace.rows; ++row) {
        const std::size_t left = static_cast<std::size_t>(row * trace.cols);
        const std::size_t right = static_cast<std::size_t>(row * trace.cols + (trace.cols - 1));
        emit_surface_segment(sorter,
                             transform.project(trace.x[left], trace.y[left], trace.z[left]),
                             transform.project(trace.x[left], trace.y[left], base_z),
                             theme.plot_border,
                             0.95,
                             0.62,
                             "solid",
                             1);
        emit_surface_segment(sorter,
                             transform.project(trace.x[right], trace.y[right], trace.z[right]),
                             transform.project(trace.x[right], trace.y[right], base_z),
                             theme.plot_border,
                             0.95,
                             0.62,
                             "solid",
                             1);
        emit_surface_segment(sorter,
                             transform.project(trace.x[left], trace.y[left], base_z),
                             transform.project(trace.x[right], trace.y[right], base_z),
                             theme.plot_border,
                             0.95,
                             0.62,
                             "solid",
                             1);
      }
    }

    if (!trace.contours && !trace.base_contours) {
      return;
    }

    const int contour_levels = std::clamp(static_cast<int>(std::lround(std::sqrt(static_cast<double>(trace.rows * trace.cols)) /
                                                                       (1.2 + 0.55 * std::max(1, trace.contour_stride)))),
                                          4,
                                          11);
    const double contour_alpha = trace.base_contours && !trace.contours ? 0.92 : 0.82;
    const double contour_width = trace.base_contours && !trace.contours ? 1.2 : 1.05;
    for (int i = 0; i < contour_levels; ++i) {
      const double t = contour_levels == 1 ? 0.5 : static_cast<double>(i + 1) / static_cast<double>(contour_levels + 1);
      const double level = trace_zmin + (trace_zmax - trace_zmin) * t;
      const std::string contour_color = detail::surface_color_3d(level, trace_zmin, trace_zmax);
      if (trace.contours) {
        for (const auto& segment : build_contour_segments(level, level)) {
          if (!segment.valid) {
            continue;
          }
          emit_surface_segment(sorter, segment.p0, segment.p1, contour_color, contour_width, contour_alpha, "solid", 2);
        }
      }
      if (trace.base_contours) {
        for (const auto& segment : build_contour_segments(level, base_z)) {
          if (!segment.valid) {
            continue;
          }
          emit_surface_segment(sorter, segment.p0, segment.p1, contour_color, contour_width + 0.1, 0.95, "solid", 2);
        }
      }
    }
  }
};

class PatchArtist3D final : public Artist3D {
public:
  void emit(DepthSorter3D& sorter,
            const Trace3D& trace,
            const PlotTransform3D& transform,
            const ResolvedTheme& theme) const override {
    const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
    if (n < 3) {
      return;
    }
    std::vector<Point2D> polygon;
    polygon.reserve(n);
    double depth_sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      const auto projected = transform.project(trace.x[i], trace.y[i], trace.z[i]);
      if (!std::isfinite(projected.x) || !std::isfinite(projected.y) || !std::isfinite(projected.depth)) {
        return;
      }
      polygon.push_back({projected.x, projected.y});
      depth_sum += projected.depth;
    }
    const double avg_depth = depth_sum / static_cast<double>(n);
    const std::string fill_color = trace.style.color.empty() ? theme.annotation_color : trace.style.color;
    sorter.add(ScenePrimitive3D{
        .depth = avg_depth,
        .layer = 0,
        .paint =
            [polygon, fill_color, alpha = std::min(1.0, trace.style.alpha)](RenderSurface2D& surface) {
              surface.fill_polygon(polygon, FillStyle2D{.color = fill_color, .alpha = alpha});
            },
    });
    std::vector<Point2D> outline = polygon;
    outline.push_back(polygon.front());
    sorter.add(ScenePrimitive3D{
        .depth = avg_depth,
        .layer = 1,
        .paint =
            [outline = std::move(outline), border = fill_color, alpha = std::min(1.0, trace.style.alpha)](RenderSurface2D& surface) {
              surface.stroke_polyline(outline,
                                      StrokeStyle2D{
                                          .color = border,
                                          .line_width = 1.1,
                                          .alpha = alpha,
                                          .line_style = "solid",
                                      });
            },
    });
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
      if (trace.filled) {
        const std::string swatch = trace.lighted ? shade_color_3d(detail::surface_color_3d(0.75, 0.0, 1.0), 1.08)
                                                 : detail::surface_color_3d(0.75, 0.0, 1.0);
        surface.fill_rect(x1, y - 4.0, x2 - x1, 8.0, FillStyle2D{.color = swatch, .alpha = 0.95}, 2.0);
      }
      surface.stroke_rect(x1, y - 4.0, x2 - x1, 8.0,
                          StrokeStyle2D{
                              .color = trace.wireframe ? theme.axis_color : theme.plot_border,
                              .line_width = trace.wireframe ? 1.0 : 0.8,
                              .alpha = 0.8,
                              .line_style = "solid",
                          },
                          2.0);
      return;
    }
    if (trace.kind == Trace3DKind::Patch3) {
      const std::string swatch = trace.style.color.empty() ? theme.annotation_color : trace.style.color;
      surface.fill_rect(x1, y - 4.0, x2 - x1, 8.0, FillStyle2D{.color = swatch, .alpha = std::min(1.0, trace.style.alpha)}, 2.0);
      surface.stroke_rect(x1, y - 4.0, x2 - x1, 8.0,
                          StrokeStyle2D{
                              .color = swatch,
                              .line_width = 0.9,
                              .alpha = std::min(1.0, trace.style.alpha),
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
  static const PatchArtist3D patch_artist{};
  switch (kind) {
    case Trace3DKind::Line3:
      return line_artist;
    case Trace3DKind::Scatter3:
      return scatter_artist;
    case Trace3DKind::Patch3:
      return patch_artist;
    case Trace3DKind::Surface:
    default:
      return surface_artist;
  }
}

} // namespace pluss::plotism
