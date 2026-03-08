// This header is intentionally included from flash.h inside namespace pluss::pl::flash.

enum class CameraPreset3D : std::uint8_t {
  Spatial,
  TopDown,
  Front,
  Side,
  Isometric,
};

enum class ThemePreset3D : std::uint8_t {
  Paper,
  Ocean,
  Slate,
  Mist,
};

enum class ScenePreset3D : std::uint8_t {
  Survey,
  Presentation,
  Studio,
  Topology,
};

enum class TracePreset3D : std::uint8_t {
  Path,
  Cloud,
  Surface,
  Wireframe,
  Spire,
  Volume,
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

struct Scene3DOptions {
  std::optional<ScenePreset3D> preset{std::nullopt};
  std::optional<ThemePreset3D> theme{std::nullopt};
  std::optional<CameraPreset3D> camera_preset{std::nullopt};
  std::optional<Camera3DOptions> camera{std::nullopt};
  std::optional<Lighting3DOptions> lighting{std::nullopt};
  std::optional<GridStyle> grid_style{std::nullopt};
};

struct Trace3DOptions {
  std::optional<TracePreset3D> preset{std::nullopt};
  std::optional<ShadeMode> shade{std::nullopt};
  std::optional<LightingMode> lighting{std::nullopt};
  std::optional<MaterialProfile> material{std::nullopt};
  std::optional<float> line_width{std::nullopt};
  std::optional<bool> transparent{std::nullopt};
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

inline Pane& theme_preset3d(Pane& pane, ThemePreset3D preset) {
  switch (preset) {
    case ThemePreset3D::Paper:
      pane.theme_tokens() = Theme::paper();
      break;
    case ThemePreset3D::Ocean:
      pane.theme_tokens() = Theme::ocean();
      break;
    case ThemePreset3D::Slate: {
      const Theme base = pane.theme_tokens();
      pane.theme_tokens() = Theme{
          "flash3d_slate",
          base.accents(),
          Rgba::from_u8(30, 36, 46),
          Rgba::from_u8(232, 237, 243),
          base.surface_palette(),
      };
      break;
    }
    case ThemePreset3D::Mist: {
      const Theme base = pane.theme_tokens();
      pane.theme_tokens() = Theme{
          "flash3d_mist",
          base.accents(),
          Rgba::from_u8(245, 248, 252),
          Rgba::from_u8(42, 52, 66),
          base.surface_palette(),
      };
      break;
    }
  }
  return pane;
}

inline Pane& scene_preset3d(Pane& pane, ScenePreset3D preset) {
  switch (preset) {
    case ScenePreset3D::Survey:
      theme_preset3d(pane, ThemePreset3D::Ocean);
      pane.grid(GridStyle::Major);
      camera_preset3d(pane, CameraPreset3D::Isometric);
      lighting3d(
          pane,
          Lighting3DOptions{
              .clear_first = true,
              .headlight = true,
              .lamp_left = true,
              .lamp_right = false,
              .sun_key = false,
          });
      break;
    case ScenePreset3D::Presentation:
      theme_preset3d(pane, ThemePreset3D::Paper);
      pane.grid(GridStyle::Major);
      camera3d(
          pane,
          Camera3DOptions{
              .azimuth_deg = -42.0,
              .elevation_deg = 24.0,
              .view_angle_deg = 28.0,
              .projection = ProjectionKind::Perspective,
          });
      lighting3d(
          pane,
          Lighting3DOptions{
              .clear_first = true,
              .headlight = true,
              .lamp_left = false,
              .lamp_right = true,
              .sun_key = true,
          });
      break;
    case ScenePreset3D::Studio:
      theme_preset3d(pane, ThemePreset3D::Slate);
      pane.grid(GridStyle::Major);
      camera_preset3d(pane, CameraPreset3D::Spatial);
      lighting3d(
          pane,
          Lighting3DOptions{
              .clear_first = true,
              .headlight = true,
              .lamp_left = true,
              .lamp_right = true,
              .sun_key = false,
          });
      break;
    case ScenePreset3D::Topology:
      theme_preset3d(pane, ThemePreset3D::Mist);
      pane.grid(GridStyle::Major);
      camera_preset3d(pane, CameraPreset3D::TopDown);
      lighting3d(
          pane,
          Lighting3DOptions{
              .clear_first = true,
              .headlight = true,
              .lamp_left = false,
              .lamp_right = false,
              .sun_key = false,
          });
      break;
  }
  return pane;
}

inline Pane& scene3d(Pane& pane, const Scene3DOptions& options) {
  if (options.preset.has_value()) {
    scene_preset3d(pane, *options.preset);
  }
  if (options.theme.has_value()) {
    theme_preset3d(pane, *options.theme);
  }
  if (options.grid_style.has_value()) {
    pane.grid(*options.grid_style);
  }
  if (options.camera_preset.has_value()) {
    camera_preset3d(pane, *options.camera_preset);
  }
  if (options.camera.has_value()) {
    camera3d(pane, *options.camera);
  }
  if (options.lighting.has_value()) {
    lighting3d(pane, *options.lighting);
  }
  return pane;
}

inline Trace& trace_preset3d(Trace& trace, TracePreset3D preset) {
  switch (preset) {
    case TracePreset3D::Path:
      trace.shade(ShadeMode::Flat).lighting(LightingMode::None).material(MaterialProfile::Default).width(2.0F);
      break;
    case TracePreset3D::Cloud:
      trace.shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Dull).transparent(true);
      break;
    case TracePreset3D::Surface:
      trace.shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Shiny);
      break;
    case TracePreset3D::Wireframe:
      trace.shade(ShadeMode::Flat).lighting(LightingMode::Flat).material(MaterialProfile::Dull).width(1.2F);
      break;
    case TracePreset3D::Spire:
      trace.shade(ShadeMode::Faceted).lighting(LightingMode::Flat).material(MaterialProfile::Metal);
      break;
    case TracePreset3D::Volume:
      trace.shade(ShadeMode::Smooth).lighting(LightingMode::Gouraud).material(MaterialProfile::Metal).transparent(true);
      break;
  }
  return trace;
}

inline Trace& trace3d(Trace& trace, const Trace3DOptions& options) {
  if (options.preset.has_value()) {
    trace_preset3d(trace, *options.preset);
  }
  if (options.shade.has_value()) {
    trace.shade(*options.shade);
  }
  if (options.lighting.has_value()) {
    trace.lighting(*options.lighting);
  }
  if (options.material.has_value()) {
    trace.material(*options.material);
  }
  if (options.line_width.has_value()) {
    trace.width(*options.line_width);
  }
  if (options.transparent.has_value()) {
    trace.transparent(*options.transparent);
  }
  return trace;
}
