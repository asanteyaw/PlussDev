#include <array>
#include <cmath>
#include <stdexcept>

#include "plot/modules/camera_3d.h"
#include "plot/modules/projector_3d.h"

namespace {

void require(bool cond, const char* msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

} // namespace

int main() {
  static constexpr std::array<std::array<double, 3>, 8> corners{{
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.0, 1.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0},
      {1.0, 0.0, 1.0},
      {1.0, 1.0, 1.0},
      {0.0, 1.0, 1.0},
  }};

  const std::array<pluss::plotism::Camera3D, 4> cameras{{
      pluss::plotism::Camera3D{.azimuth_deg = 132.0, .elevation_deg = 24.0, .distance = 3.6, .projection = pluss::plotism::ProjectionMode::Perspective},
      pluss::plotism::Camera3D{.azimuth_deg = 38.0, .elevation_deg = 62.0, .distance = 2.8, .projection = pluss::plotism::ProjectionMode::Perspective},
      pluss::plotism::Camera3D{.azimuth_deg = -28.0, .elevation_deg = 18.0, .distance = 4.5, .projection = pluss::plotism::ProjectionMode::Perspective},
      pluss::plotism::Camera3D{.azimuth_deg = 120.0, .elevation_deg = 35.0, .distance = 3.0, .projection = pluss::plotism::ProjectionMode::Orthographic},
  }};

  for (const auto& camera : cameras) {
    const double extent = pluss::plotism::detail::fitted_projection_extent_3d(camera);
    require(std::isfinite(extent), "expected finite 3d projection extent");
    require(extent > 0.0 && extent <= 0.94, "expected fitted 3d extent to stay within safe viewport bounds");

    for (const auto& corner : corners) {
      const auto projected = pluss::plotism::detail::project_normalized_3d(
          corner[0], corner[1], corner[2], camera, extent, 100.0, 80.0, 640.0, 480.0);
      require(std::isfinite(projected.x) && std::isfinite(projected.y), "expected finite projected cube corner");
      require(projected.x >= 100.0 - 1e-6 && projected.x <= 740.0 + 1e-6,
              "expected projected 3d x coordinate to stay inside viewport");
      require(projected.y >= 80.0 - 1e-6 && projected.y <= 560.0 + 1e-6,
              "expected projected 3d y coordinate to stay inside viewport");
    }
  }

  return 0;
}
