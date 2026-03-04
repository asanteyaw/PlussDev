#pragma once

#include "plot/modules/backend/raster_surface_2d.h"
#include "plot/modules/backend/render_2d_shared.h"

namespace pluss::plotism {
namespace detail {

inline std::string render_png_2d(const Figure2DView& fig, const RenderTarget& target) {
  RasterSurface2D surface(target.width, target.height, recommended_png_supersample(target.width, target.height));
  render_2d_scene(surface, fig, target);
  return surface.finish();
}

} // namespace detail
} // namespace pluss::plotism
