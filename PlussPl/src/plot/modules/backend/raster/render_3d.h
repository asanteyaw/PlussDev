#pragma once

#include "plot/modules/backend/raster_surface_2d.h"
#include "plot/modules/backend/render_3d_shared.h"

namespace pluss::plotism {
namespace detail {

inline std::string render_png_3d(const Figure3DView& fig, const RenderTarget& target) {
  RasterSurface2D surface(target.width, target.height, 2);
  render_3d_scene(surface, fig, target);
  return surface.finish();
}

} // namespace detail
} // namespace pluss::plotism
