#pragma once

#include "table/plot/DataFramePlot.h"

namespace pluss::table::plot::detail {

void apply_3d_trace_options(pluss::pl::Trace& trace, const Plot3DTraceOptions& options);
void apply_3d_theme_preset(pluss::pl::Pane& pane, Plot3DThemePreset preset);
void apply_3d_scene_options(pluss::pl::Pane& pane, const Plot3DSceneOptions& options);

} // namespace pluss::table::plot::detail
