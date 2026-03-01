#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "plot/modules/axes.h"
#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/camera_3d.h"

namespace pluss::plotism {
namespace detail {

struct ProjectedPoint3D {
  double x{std::numeric_limits<double>::quiet_NaN()};
  double y{std::numeric_limits<double>::quiet_NaN()};
  double depth{std::numeric_limits<double>::quiet_NaN()};
};

inline double scalar_color_component_3d(double a, double b, double t) {
  return a + (b - a) * std::clamp(t, 0.0, 1.0);
}

inline std::string hex_color_from_rgb_3d(int r, int g, int b) {
  const auto to_hex = [](int value) -> char {
    static const char* digits = "0123456789abcdef";
    return digits[std::clamp(value, 0, 15)];
  };
  std::string out = "#000000";
  out[1] = to_hex((r >> 4) & 0xF);
  out[2] = to_hex(r & 0xF);
  out[3] = to_hex((g >> 4) & 0xF);
  out[4] = to_hex(g & 0xF);
  out[5] = to_hex((b >> 4) & 0xF);
  out[6] = to_hex(b & 0xF);
  return out;
}

inline std::string surface_color_3d(double value, double lo, double hi) {
  struct Stop {
    double t;
    int r;
    int g;
    int b;
  };
  static constexpr std::array<Stop, 5> stops{{
      {0.0, 11, 60, 93},
      {0.25, 29, 119, 184},
      {0.5, 105, 178, 199},
      {0.75, 244, 192, 149},
      {1.0, 196, 69, 54},
  }};
  if (!std::isfinite(value)) {
    return "#e5e7eb";
  }
  if (!(hi > lo)) {
    return "#6fb1c7";
  }
  const double t = std::clamp((value - lo) / (hi - lo), 0.0, 1.0);
  for (std::size_t i = 1; i < stops.size(); ++i) {
    if (t <= stops[i].t) {
      const double span = std::max(1e-12, stops[i].t - stops[i - 1].t);
      const double local = (t - stops[i - 1].t) / span;
      const int r = static_cast<int>(std::round(scalar_color_component_3d(stops[i - 1].r, stops[i].r, local)));
      const int g = static_cast<int>(std::round(scalar_color_component_3d(stops[i - 1].g, stops[i].g, local)));
      const int b = static_cast<int>(std::round(scalar_color_component_3d(stops[i - 1].b, stops[i].b, local)));
      return hex_color_from_rgb_3d(r, g, b);
    }
  }
  return hex_color_from_rgb_3d(stops.back().r, stops.back().g, stops.back().b);
}

inline std::array<double, 3> rotate_camera_3d(double xn, double yn, double zn, const Camera3D& camera) {
  const double x0 = xn - 0.5;
  const double y0 = yn - 0.5;
  const double z0 = zn - 0.5;
  const double az = camera.azimuth_deg * 3.14159265358979323846 / 180.0;
  const double el = camera.elevation_deg * 3.14159265358979323846 / 180.0;

  const double x1 = std::cos(az) * x0 - std::sin(az) * y0;
  const double y1 = std::sin(az) * x0 + std::cos(az) * y0;
  const double z1 = z0;

  const double x2 = x1;
  const double y2 = std::cos(el) * y1 - std::sin(el) * z1;
  const double z2 = std::sin(el) * y1 + std::cos(el) * z1;
  return {x2, y2, z2};
}

inline ProjectedPoint3D project_normalized_3d(double xn,
                                              double yn,
                                              double zn,
                                              const Camera3D& camera,
                                              double left,
                                              double top,
                                              double width,
                                              double height) {
  const auto rotated = rotate_camera_3d(xn, yn, zn, camera);
  double scale = 1.0;
  if (camera.projection == ProjectionMode::Perspective) {
    scale = camera.distance / std::max(0.6, camera.distance - rotated[1]);
  }
  const double extent = 0.88;
  const double px = left + width * (0.5 + rotated[0] * scale * extent);
  const double py = top + height * (0.5 - rotated[2] * scale * extent);
  return ProjectedPoint3D{.x = px, .y = py, .depth = rotated[1]};
}

inline ProjectedPoint3D project_value_3d(double x,
                                         double y,
                                         double z,
                                         const Axis& x_axis,
                                         const Axis& y_axis,
                                         const Axis& z_axis,
                                         double xmin,
                                         double xmax,
                                         double ymin,
                                         double ymax,
                                         double zmin,
                                         double zmax,
                                         const Camera3D& camera,
                                         double left,
                                         double top,
                                         double width,
                                         double height) {
  const double tx = axis_transform_2d(x, x_axis);
  const double ty = axis_transform_2d(y, y_axis);
  const double tz = axis_transform_2d(z, z_axis);
  if (!std::isfinite(tx) || !std::isfinite(ty) || !std::isfinite(tz)) {
    return {};
  }
  double xn = (tx - xmin) / (xmax - xmin);
  double yn = (ty - ymin) / (ymax - ymin);
  double zn = (tz - zmin) / (zmax - zmin);
  if (x_axis.inverted) xn = 1.0 - xn;
  if (y_axis.inverted) yn = 1.0 - yn;
  if (z_axis.inverted) zn = 1.0 - zn;
  return project_normalized_3d(xn, yn, zn, camera, left, top, width, height);
}

} // namespace detail
} // namespace pluss::plotism
