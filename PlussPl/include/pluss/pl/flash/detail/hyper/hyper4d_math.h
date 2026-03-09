#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

struct HyperVector4 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
  double w{0.0};
};

struct HyperProjectedPoint {
  double x{0.0};
  double y{0.0};
  double z{0.0};
  double w{0.0};
};

[[nodiscard]] inline double deg_to_rad(double deg) noexcept {
  return deg * (3.14159265358979323846 / 180.0);
}

inline void rotate_plane(double& a, double& b, double radians) noexcept {
  if (std::abs(radians) < 1e-15) {
    return;
  }
  const double c = std::cos(radians);
  const double s = std::sin(radians);
  const double ra = c * a - s * b;
  const double rb = s * a + c * b;
  a = ra;
  b = rb;
}

inline HyperVector4 rotate4(
    HyperVector4 v,
    double rot_xy_deg,
    double rot_xz_deg,
    double rot_xw_deg,
    double rot_yz_deg,
    double rot_yw_deg,
    double rot_zw_deg) noexcept {
  rotate_plane(v.x, v.y, deg_to_rad(rot_xy_deg));
  rotate_plane(v.x, v.z, deg_to_rad(rot_xz_deg));
  rotate_plane(v.x, v.w, deg_to_rad(rot_xw_deg));
  rotate_plane(v.y, v.z, deg_to_rad(rot_yz_deg));
  rotate_plane(v.y, v.w, deg_to_rad(rot_yw_deg));
  rotate_plane(v.z, v.w, deg_to_rad(rot_zw_deg));
  return v;
}

[[nodiscard]] inline bool inside_slice(
    const std::optional<Range1d>& w_slice,
    double w) noexcept {
  return !w_slice.has_value() || (w >= w_slice->low && w <= w_slice->high);
}

enum class HyperProjectionKind : std::uint8_t {
  Orthographic,
  PerspectiveW,
};

[[nodiscard]] inline std::optional<HyperProjectedPoint> project4(
    const HyperVector4& v,
    HyperProjectionKind projection,
    double w_perspective_distance) noexcept {
  if (projection == HyperProjectionKind::Orthographic) {
    return HyperProjectedPoint{v.x, v.y, v.z, v.w};
  }

  const double distance = std::max(1e-6, w_perspective_distance);
  const double denom = distance - v.w;
  if (std::abs(denom) < 1e-9) {
    return std::nullopt;
  }
  const double scale = distance / denom;
  if (!std::isfinite(scale) || scale <= 0.0) {
    return std::nullopt;
  }
  return HyperProjectedPoint{v.x * scale, v.y * scale, v.z * scale, v.w};
}

[[nodiscard]] inline std::vector<HyperProjectedPoint> project4_batch(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& z,
    const std::vector<double>& w,
    double rot_xy_deg,
    double rot_xz_deg,
    double rot_xw_deg,
    double rot_yz_deg,
    double rot_yw_deg,
    double rot_zw_deg,
    HyperProjectionKind projection,
    double w_perspective_distance,
    const std::optional<Range1d>& w_slice) {
  const std::size_t n = x.size();
  std::vector<HyperProjectedPoint> out;
  out.reserve(n);

  for (const auto i : std::views::iota(std::size_t{0}, n)) {
    const HyperVector4 rotated = rotate4(
        HyperVector4{x[i], y[i], z[i], w[i]},
        rot_xy_deg,
        rot_xz_deg,
        rot_xw_deg,
        rot_yz_deg,
        rot_yw_deg,
        rot_zw_deg);
    if (!inside_slice(w_slice, rotated.w)) {
      continue;
    }
    const std::optional<HyperProjectedPoint> projected =
        project4(rotated, projection, w_perspective_distance);
    if (!projected.has_value()) {
      continue;
    }
    out.push_back(*projected);
  }

  return out;
}
