#include "pluss/pl/plusspl.h"

#include <cmath>

namespace pluss::pl {

Lens& Lens::look(double azimuth_deg, double elevation_deg) noexcept {
  azimuth_deg_ = azimuth_deg;
  elevation_deg_ = elevation_deg;
  return *this;
}

Lens& Lens::spin(double delta_azimuth_deg, double delta_elevation_deg) noexcept {
  azimuth_deg_ += delta_azimuth_deg;
  elevation_deg_ += delta_elevation_deg;
  return *this;
}

Lens& Lens::target_at(Vec3d target) noexcept {
  target_ = target;
  return *this;
}

Lens& Lens::up(Vec3d up_vector) noexcept {
  up_vector_ = up_vector;
  return *this;
}

Lens& Lens::project(ProjectionKind projection) noexcept {
  projection_ = projection;
  return *this;
}

Lens& Lens::view_angle(double degrees) noexcept {
  view_angle_deg_ = degrees;
  return *this;
}

Lens Lens::planar() noexcept {
  Lens lens;
  lens.azimuth_deg_ = 0.0;
  lens.elevation_deg_ = 90.0;
  lens.up_vector_ = Vec3d{0.0, 1.0, 0.0};
  lens.projection_ = ProjectionKind::Orthographic;
  return lens;
}

Lens Lens::spatial() noexcept {
  Lens lens;
  lens.azimuth_deg_ = -37.5;
  lens.elevation_deg_ = 30.0;
  lens.up_vector_ = Vec3d{0.0, 0.0, 1.0};
  lens.projection_ = ProjectionKind::Perspective;
  return lens;
}

LightRig& LightRig::clear() noexcept {
  lights_.clear();
  return *this;
}

LightRig& LightRig::add(LightSpec light) {
  lights_.push_back(light);
  return *this;
}

LightRig& LightRig::headlight() noexcept {
  lights_.push_back(LightSpec{0.0, 0.0, LightSpace::Camera, LightStyle::Local});
  return *this;
}

LightRig& LightRig::lamp_right() noexcept {
  lights_.push_back(LightSpec{30.0, 30.0, LightSpace::Camera, LightStyle::Local});
  return *this;
}

LightRig& LightRig::lamp_left() noexcept {
  lights_.push_back(LightSpec{-30.0, 30.0, LightSpace::Camera, LightStyle::Local});
  return *this;
}

LightRig& LightRig::sun_key() noexcept {
  lights_.push_back(LightSpec{-20.0, 35.0, LightSpace::Camera, LightStyle::Infinite});
  lights_.push_back(LightSpec{120.0, 15.0, LightSpace::Camera, LightStyle::Infinite});
  return *this;
}

Trace::Trace(TraceKind kind, TracePayload payload)
    : kind_(kind), payload_(std::move(payload)) {}

Trace& Trace::label(std::string label) {
  label_ = std::move(label);
  return *this;
}

Trace& Trace::width(float width) noexcept {
  if (!(std::isfinite(width) && width > 0.0F)) {
    return *this;
  }
  stroke_width_ = width;
  return *this;
}

Trace& Trace::stroke(Rgba color) noexcept {
  stroke_color_ = color;
  return *this;
}

Trace& Trace::palette(PaletteRamp palette) {
  palette_ = std::move(palette);
  return *this;
}

Trace& Trace::color_limits(Range1d limits) noexcept {
  if (!(std::isfinite(limits.low) && std::isfinite(limits.high) && limits.high > limits.low)) {
    return *this;
  }
  color_limits_ = limits;
  return *this;
}

Trace& Trace::shade(ShadeMode mode) noexcept {
  shade_mode_ = mode;
  return *this;
}

Trace& Trace::lighting(LightingMode mode) noexcept {
  lighting_mode_ = mode;
  return *this;
}

Trace& Trace::material(MaterialProfile profile) noexcept {
  material_profile_ = profile;
  return *this;
}

Trace& Trace::transparent(bool enable) noexcept {
  transparent_ = enable;
  return *this;
}

Trace& Trace::annotate_levels(bool enable) noexcept {
  annotate_levels_ = enable;
  return *this;
}

Trace& Trace::hide() noexcept {
  visible_ = false;
  return *this;
}

Trace& Trace::show() noexcept {
  visible_ = true;
  return *this;
}

} // namespace pluss::pl
