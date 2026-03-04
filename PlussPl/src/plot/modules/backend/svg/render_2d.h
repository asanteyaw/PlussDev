#pragma once

#include "plot/modules/backend/render_2d_shared.h"
#include "plot/modules/backend/svg/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_svg_2d(const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save_svg: width/height too small");
  }

  SvgRenderSurface2D surface;
  render_2d_scene(surface, fig, target);

  std::ostringstream svg;
  const double physical_width = static_cast<double>(target.width) / std::max(1.0, target.dpi);
  const double physical_height = static_cast<double>(target.height) / std::max(1.0, target.dpi);
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << format_double(physical_width, 6)
      << "in\" height=\"" << format_double(physical_height, 6)
      << "in\" data-plusspl-dpi=\"" << format_double(target.dpi, 6)
      << "\" viewBox=\"0 0 " << target.width << " " << target.height << "\">\n";
  svg << surface.finish();
  svg << "</svg>\n";
  return svg.str();
}
} // namespace detail
} // namespace pluss::plotism
