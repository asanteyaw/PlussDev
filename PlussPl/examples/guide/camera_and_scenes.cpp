// One surface, many viewpoints: the camera, the lights, the theme and the scene presets that turn a bare 3D plot into a presentable one.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  // A smooth test surface: z = sin(r) / r over a 40 x 40 grid.
  const std::int64_t n = 40;
  auto axis = pluss::series::linspace(-8.0, 8.0, n, pluss::ScalarType::Float64);
  auto gx = axis.unsqueeze(0).expand({n, n});
  auto gy = axis.unsqueeze(1).expand({n, n});
  auto r = (gx.square() + gy.square()).sqrt() + 1e-9;
  auto z = r.sin() / r;
  std::cout << "surface " << n << " x " << n << ", height from " << z.min().item<double>() << " to " << z.max().item<double>() << "\n";

  auto fig = studio("camera_and_scenes");
  fig.mosaic(2, 3);
  fig.headline("The same surface, six ways");
  auto &spatial = fig.pane3("Default camera");
  spatial.sheet(axis, axis, z);
  auto &top = fig.pane3("Top-down preset");
  top.sheet(axis, axis, z);
  flash::camera_preset3d(top, flash::CameraPreset3D::TopDown);
  auto &iso = fig.pane3("Custom angles, perspective");
  iso.sheet(axis, axis, z);
  flash::camera3d(iso, 20.0, 35.0, ProjectionKind::Perspective, 40.0);
  auto &lit = fig.pane3("Lamps on");
  lit.sheet(axis, axis, z);
  flash::lighting3d(lit, flash::Lighting3DOptions{.clear_first = true, .headlight = false, .lamp_left = true, .lamp_right = true, .sun_key = true});
  auto &themed = fig.pane3("Slate theme preset");
  themed.sheet(axis, axis, z);
  flash::theme_preset3d(themed, flash::ThemePreset3D::Slate);
  auto &scene = fig.pane3("Presentation scene with trace preset");
  auto &trace = scene.sheet(axis, axis, z);
  flash::scene_preset3d(scene, flash::ScenePreset3D::Presentation);
  flash::trace_preset3d(trace, flash::TracePreset3D::Wireframe);
  fig.show();

  // The option structs do the same in one call: scene3d for the pane, trace3d for the trace.
  auto combined = studio("camera_scene_options");
  auto &pane = combined.pane3("scene3d and trace3d");
  auto &surface = pane.sheet(axis, axis, z);
  flash::scene3d(pane, flash::Scene3DOptions{.preset = flash::ScenePreset3D::Survey, .camera_preset = flash::CameraPreset3D::Isometric});
  flash::trace3d(surface, flash::Trace3DOptions{.preset = flash::TracePreset3D::Surface, .transparent = true});
  combined.show();
  std::cout << "two figures written\n";
}
