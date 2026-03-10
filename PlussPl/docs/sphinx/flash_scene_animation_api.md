# Flash 3D And Geometric 4D Projection Rendering API

This page covers the scene/trace controls for 3D and the animation/projection helpers for 3D and geometric 4D projection rendering.

Header:

```cpp
#include <pluss/pl/flash.h>
```

## 3D scene controls

Types:

```cpp
enum class CameraPreset3D { Spatial, TopDown, Front, Side, Isometric };
enum class ThemePreset3D { Paper, Ocean, Slate, Mist };
enum class ScenePreset3D { Survey, Presentation, Studio, Topology };
enum class TracePreset3D { Path, Cloud, Surface, Wireframe, Spire, Volume };

struct Camera3DOptions {
  double azimuth_deg;
  double elevation_deg;
  double view_angle_deg;
  ProjectionKind projection;
};

struct Lighting3DOptions {
  bool clear_first;
  bool headlight;
  bool lamp_left;
  bool lamp_right;
  bool sun_key;
};

struct Scene3DOptions {
  std::optional<ScenePreset3D> preset;
  std::optional<ThemePreset3D> theme;
  std::optional<CameraPreset3D> camera_preset;
  std::optional<Camera3DOptions> camera;
  std::optional<Lighting3DOptions> lighting;
  std::optional<GridStyle> grid_style;
};

struct Trace3DOptions {
  std::optional<TracePreset3D> preset;
  std::optional<ShadeMode> shade;
  std::optional<LightingMode> lighting;
  std::optional<MaterialProfile> material;
  std::optional<float> line_width;
  std::optional<bool> transparent;
};
```

Functions:

```cpp
Pane& camera3d(Pane&, const Camera3DOptions&);
Pane& camera3d(Pane&, double azimuth_deg, double elevation_deg,
               ProjectionKind projection = ProjectionKind::Orthographic,
               double view_angle_deg = 30.0);
Pane& camera_preset3d(Pane&, CameraPreset3D);
Pane& lighting3d(Pane&, const Lighting3DOptions&);
Pane& theme_preset3d(Pane&, ThemePreset3D);
Pane& scene_preset3d(Pane&, ScenePreset3D);
Pane& scene3d(Pane&, const Scene3DOptions&);
Trace& trace_preset3d(Trace&, TracePreset3D);
Trace& trace3d(Trace&, const Trace3DOptions&);
```

## 3D motion simulation

Types:

```cpp
enum class Anim3SweepPlane { XY, XZ, YZ };

struct Animation3Options {
  std::string title;
  std::int64_t width;
  std::int64_t height;
  Anim3SweepPlane sweep_plane;
  double base_rot_xy_deg;
  double base_rot_xz_deg;
  double base_rot_yz_deg;
  double sweep_speed_deg_per_sec;
  double camera_distance;
  double camera_yaw_deg;
  double camera_pitch_deg;
  double min_perspective_denom;
  double plot_padding_fraction;
  bool progressive_reveal;
  double reveal_points_per_sec;
  bool connect_path;
  double line_width;
  double point_radius;
  bool enable_snapshot;
  std::string snapshot_prefix;
};
```

Function:

```cpp
template <class X, class Y, class Z>
bool animate3(X&& x, Y&& y, Z&& z, const Animation3Options& options = {});
```

Behavior:

- renders live simulation in native webview backend used by `.show()`
- supports play/pause, speed control, snapshot capture
- intended for interactive exploration (not embedded into static PDF)

## Geometric 4D projection rendering

Types:

```cpp
enum class HyperProjection { Orthographic, PerspectiveW };

struct HyperPlotOptions {
  HyperProjection projection;
  double rot_xy_deg;
  double rot_xz_deg;
  double rot_xw_deg;
  double rot_yz_deg;
  double rot_yw_deg;
  double rot_zw_deg;
  double w_perspective_distance;
  std::optional<Range1d> w_slice;
  double point_size;
};

enum class HyperSweepPlane { XY, XZ, XW, YZ, YW, ZW };
struct HyperSweepOptions {
  std::int64_t frames;
  HyperSweepPlane plane;
  double start_deg;
  double end_deg;
  bool include_endpoint;
};
```

Functions:

```cpp
std::vector<HyperPlotOptions> hyper_animation_sequence(
    const HyperPlotOptions& base,
    const HyperSweepOptions& options = {});

template <class X, class Y, class Z, class W>
Trace& path4(Pane&, X&& x, Y&& y, Z&& z, W&& w,
             const HyperPlotOptions& options = {}, std::string label = {});

template <class X, class Y, class Z, class W>
Trace& cloud4(Pane&, X&& x, Y&& y, Z&& z, W&& w,
              const HyperPlotOptions& options = {}, std::string label = {});
```

## Geometric 4D motion simulation and frame export

Types:

```cpp
enum class HyperAnimateSweepPlane { XY, XZ, XW, YZ, YW, ZW };

struct HyperAnimationOptions {
  std::string title;
  std::int64_t width;
  std::int64_t height;
  HyperProjection projection;
  HyperAnimateSweepPlane sweep_plane;
  double base_rot_xy_deg;
  double base_rot_xz_deg;
  double base_rot_xw_deg;
  double base_rot_yz_deg;
  double base_rot_yw_deg;
  double base_rot_zw_deg;
  double sweep_speed_deg_per_sec;
  double w_perspective_distance;
  double slice_min;
  double slice_max;
  double slice_default;
  double camera_distance;
  double camera_yaw_deg;
  double camera_pitch_deg;
  double min_perspective_denom;
  double plot_padding_fraction;
  double point_radius_base;
  double point_radius_min;
  double point_radius_max;
  bool enable_snapshot;
  std::string snapshot_prefix;
};

enum class AnimationFrameFormat { Png, Svg, Pdf };

struct HyperFrameExportOptions {
  std::filesystem::path output_dir;
  std::string file_prefix;
  std::string title;
  AnimationFrameFormat format;
  std::int64_t width;
  std::int64_t height;
  std::int64_t dpi;
  Theme theme;
  HyperPlotOptions plot;
  HyperSweepOptions sweep;
};
```

Functions:

```cpp
template <class X, class Y, class Z, class W>
bool animate4(X&& x, Y&& y, Z&& z, W&& w,
              const HyperAnimationOptions& options = {});

template <class X, class Y, class Z, class W>
std::int64_t export4_frames(X&& x, Y&& y, Z&& z, W&& w,
                            const HyperFrameExportOptions& options = {});
```

## Minimal usage

```cpp
#include <pluss/pl/flash.h>

using namespace pluss::pl;
using namespace pluss::pl::flash;

Studio fig("scene_demo", Theme::paper());
auto& pane = fig.pane3("surface");

scene3d(pane, Scene3DOptions{
                 .preset = ScenePreset3D::Presentation,
             });

auto& tr = sheet(
    pane,
    std::vector<double>{0, 1, 2, 0, 1, 2},
    std::vector<double>{0, 0, 0, 1, 1, 1},
    std::vector<double>{0.0, 0.2, 0.4, 0.3, 0.6, 0.8});
trace3d(tr, Trace3DOptions{
               .preset = TracePreset3D::Surface,
           });

fig.save_pdf("flash_scene_demo.pdf");
```
