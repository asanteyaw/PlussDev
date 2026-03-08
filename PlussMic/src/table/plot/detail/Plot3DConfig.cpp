#include "table/plot/detail/Plot3DConfig.h"

namespace pluss::table::plot::detail {

void apply_3d_trace_options(pluss::pl::Trace& trace, const Plot3DTraceOptions& options) {
  pluss::pl::flash::Trace3DOptions trace_options;
  trace_options.preset = options.preset;
  trace_options.shade = options.shade;
  trace_options.lighting = options.lighting;
  trace_options.material = options.material;
  trace_options.line_width = options.line_width;
  trace_options.transparent = options.transparent;
  pluss::pl::flash::trace3d(trace, trace_options);
}

void apply_3d_scene_options(pluss::pl::Pane& pane, const Plot3DSceneOptions& options) {
  pluss::pl::flash::Scene3DOptions scene_options;
  scene_options.preset = options.preset;
  scene_options.theme = options.theme;
  scene_options.camera_preset = options.camera_preset;
  scene_options.camera = options.camera;
  scene_options.lighting = options.lighting;
  scene_options.grid_style = options.grid_style;
  pluss::pl::flash::scene3d(pane, scene_options);
}

} // namespace pluss::table::plot::detail
