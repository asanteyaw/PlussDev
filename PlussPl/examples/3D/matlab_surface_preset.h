#pragma once

#include <algorithm>
#include <cmath>

#include "pluss/plot/plot.h"

namespace plusspl::examples {

inline void apply_matlab_surface_preset(pluss::plotism::Figure3D& fig,
                                        double x_span,
                                        double y_span,
                                        double z_span) {
  const double safe_x = std::max(1e-9, x_span);
  const double safe_y = std::max(1e-9, y_span);
  const double safe_z = std::max(1e-9, z_span);
  const double xy_ratio = safe_x / safe_y;
  const double z_vs_plane = safe_z / std::max(safe_x, safe_y);

  fig.theme("paper").font_family("Helvetica Neue");
  fig.view(-37.5, 30.0)
      .camera_distance(std::clamp(4.35 - 0.14 * std::log1p(xy_ratio), 3.7, 4.55))
      .projection(pluss::plotism::ProjectionMode::Perspective)
      .data_aspect(safe_x, safe_y, safe_z)
      .plot_box_aspect(1.18 + 0.1 * std::log1p(xy_ratio),
                       1.0,
                       std::clamp(0.68 + 0.82 * std::sqrt(std::max(0.0, z_vs_plane)), 0.58, 1.0))
      .surface_wireframe(true)
      .surface_contours(false)
      .surface_lighting(false);

  fig.xaxis()
      .set_tick_locator(pluss::plotism::TickLocator{.kind = pluss::plotism::TickLocatorKind::Linear, .approx_count = 5})
      .set_minor_tick_count(1)
      .set_tick_formatter(pluss::plotism::TickFormatter{.kind = pluss::plotism::TickFormatterKind::Numeric, .precision = 3});
  fig.yaxis()
      .set_tick_locator(pluss::plotism::TickLocator{.kind = pluss::plotism::TickLocatorKind::Linear, .approx_count = 5})
      .set_minor_tick_count(1)
      .set_tick_formatter(pluss::plotism::TickFormatter{.kind = pluss::plotism::TickFormatterKind::Numeric, .precision = 3});
  fig.zaxis()
      .set_tick_locator(pluss::plotism::TickLocator{.kind = pluss::plotism::TickLocatorKind::Linear, .approx_count = 5})
      .set_minor_tick_count(1)
      .set_tick_formatter(pluss::plotism::TickFormatter{.kind = pluss::plotism::TickFormatterKind::Numeric, .precision = 4});
}

} // namespace plusspl::examples
