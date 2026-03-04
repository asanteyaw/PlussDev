#pragma once

#include <cstdint>

namespace pluss::plotism {

enum class ProjectionMode : std::uint8_t {
  Perspective = 0,
  Orthographic = 1,
};

struct Camera3D {
  double azimuth_deg{-37.5};
  double elevation_deg{30.0};
  double distance{3.4};
  ProjectionMode projection{ProjectionMode::Perspective};
};

} // namespace pluss::plotism
