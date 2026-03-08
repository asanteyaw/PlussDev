#include "table/plot/detail/Plot3DConfig.h"

namespace pluss::table::plot::detail {

void apply_3d_trace_options(pluss::pl::Trace& trace, const Plot3DTraceOptions& options) {
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
}

void apply_3d_theme_preset(pluss::pl::Pane& pane, Plot3DThemePreset preset) {
  switch (preset) {
    case Plot3DThemePreset::Paper:
      pane.theme_tokens() = pluss::pl::themes::paper();
      return;
    case Plot3DThemePreset::Ocean:
      pane.theme_tokens() = pluss::pl::themes::ocean();
      return;
    case Plot3DThemePreset::Dark: {
      const auto base = pane.theme_tokens();
      pane.theme_tokens() = pluss::pl::Theme{
          "plot3d_dark",
          base.accents(),
          pluss::pl::Rgba::from_u8(18, 24, 33),
          pluss::pl::Rgba::from_u8(236, 240, 245),
          base.surface_palette(),
      };
      return;
    }
  }
}

void apply_3d_scene_options(pluss::pl::Pane& pane, const Plot3DSceneOptions& options) {
  if (options.theme.has_value()) {
    apply_3d_theme_preset(pane, *options.theme);
  }
  if (options.grid_style.has_value()) {
    pane.grid(*options.grid_style);
  }
  if (options.camera_preset.has_value()) {
    pluss::pl::flash::camera_preset3d(pane, *options.camera_preset);
  }
  if (options.camera.has_value()) {
    pluss::pl::flash::camera3d(pane, *options.camera);
  }
  if (options.lighting.has_value()) {
    pluss::pl::flash::lighting3d(pane, *options.lighting);
  }
}

} // namespace pluss::table::plot::detail
