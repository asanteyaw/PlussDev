// This header is intentionally included from flash.h inside namespace pluss::pl::flash.

enum class CameraPreset3D : std::uint8_t {
  Spatial,
  TopDown,
  Front,
  Side,
  Isometric,
};

struct Camera3DOptions {
  double azimuth_deg{-37.5};
  double elevation_deg{30.0};
  double view_angle_deg{30.0};
  ProjectionKind projection{ProjectionKind::Orthographic};
};

struct Lighting3DOptions {
  bool clear_first{true};
  bool headlight{true};
  bool lamp_left{false};
  bool lamp_right{false};
  bool sun_key{false};
};

inline Pane& camera3d(Pane& pane, const Camera3DOptions& options) {
  pane.lens()
      .look(options.azimuth_deg, options.elevation_deg)
      .project(options.projection)
      .view_angle(options.view_angle_deg);
  return pane;
}

inline Pane& camera3d(
    Pane& pane,
    double azimuth_deg,
    double elevation_deg,
    ProjectionKind projection = ProjectionKind::Orthographic,
    double view_angle_deg = 30.0) {
  return camera3d(
      pane,
      Camera3DOptions{
          .azimuth_deg = azimuth_deg,
          .elevation_deg = elevation_deg,
          .view_angle_deg = view_angle_deg,
          .projection = projection,
      });
}

inline Pane& camera_preset3d(Pane& pane, CameraPreset3D preset) {
  switch (preset) {
    case CameraPreset3D::TopDown:
      return camera3d(pane, 0.0, 90.0, ProjectionKind::Orthographic, 30.0);
    case CameraPreset3D::Front:
      return camera3d(pane, 0.0, 0.0, ProjectionKind::Orthographic, 30.0);
    case CameraPreset3D::Side:
      return camera3d(pane, 90.0, 0.0, ProjectionKind::Orthographic, 30.0);
    case CameraPreset3D::Isometric:
      return camera3d(pane, 45.0, 35.2643897, ProjectionKind::Orthographic, 30.0);
    case CameraPreset3D::Spatial:
    default:
      return camera3d(pane, -37.5, 30.0, ProjectionKind::Orthographic, 30.0);
  }
}

inline Pane& lighting3d(Pane& pane, const Lighting3DOptions& options) {
  if (options.clear_first) {
    pane.lights().clear();
  }
  if (options.headlight) {
    pane.lights().headlight();
  }
  if (options.lamp_left) {
    pane.lights().lamp_left();
  }
  if (options.lamp_right) {
    pane.lights().lamp_right();
  }
  if (options.sun_key) {
    pane.lights().sun_key();
  }
  return pane;
}
