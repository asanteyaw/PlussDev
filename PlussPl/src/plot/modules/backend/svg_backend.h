#pragma once

#include <memory>
#include <stdexcept>

#include "plot/modules/backend.h"
#include "plot/modules/backend/svg/render_2d.h"
#include "plot/modules/backend/svg/render_3d.h"

namespace pluss::plotism {

class SvgBackend final : public IBackend {
public:
  void save(const Figure2DView& figure, const RenderTarget& target) const override {
    if (target.format == OutputFormat::Png) {
      throw std::runtime_error(
          "plotism::Figure2D::save_png is not enabled in this build. Use save_svg() or wire a raster backend.");
    }
    if (target.format == OutputFormat::Pdf) {
      throw std::runtime_error(
          "plotism::Figure2D::save_pdf requires a PDF-capable backend. Use make_default_backend() or make_pdf_backend().");
    }
    detail::write_text_file(target.path, detail::render_svg_2d(figure, target));
  }

  void save(const Figure3DView& figure, const RenderTarget& target) const override {
    if (target.format == OutputFormat::Png) {
      throw std::runtime_error(
          "plotism::Figure3D::save_png is not enabled in this build. Use save_svg() or wire a raster backend.");
    }
    if (target.format == OutputFormat::Pdf) {
      throw std::runtime_error(
          "plotism::Figure3D::save_pdf requires a PDF-capable backend. Use make_default_backend() or make_pdf_backend().");
    }
    detail::write_text_file(target.path, detail::render_svg_3d(figure, target));
  }
};

inline std::shared_ptr<const IBackend> make_svg_backend() {
  static const auto backend = std::make_shared<const SvgBackend>();
  return backend;
}

} // namespace pluss::plotism
