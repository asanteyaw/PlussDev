#pragma once

#include "plot/modules/backend/pdf/primitives.h"
#include "plot/modules/backend/render_2d_shared.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_pdf_2d(const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save_pdf: width/height too small");
  }

  PdfRenderSurface2D surface(target.width, target.height);
  render_2d_scene(surface, fig, target);
  return build_pdf_document(target.width, target.height, surface.finish(), target.dpi);
}

} // namespace detail
} // namespace pluss::plotism
