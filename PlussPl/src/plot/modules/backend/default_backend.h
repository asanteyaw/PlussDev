#pragma once

#include <memory>
#include <stdexcept>

#include "plot/modules/backend.h"
#include "plot/modules/backend/pdf_backend.h"
#include "plot/modules/backend/raster_backend.h"
#include "plot/modules/backend/svg_backend.h"

namespace pluss::plotism {

class DefaultBackend final : public IBackend {
public:
  DefaultBackend() : svg_(make_svg_backend()), pdf_(make_pdf_backend()), raster_(make_raster_backend()) {}

  void save(const Figure2DView& figure, const RenderTarget& target) const override {
    switch (target.format) {
      case OutputFormat::Svg:
        svg_->save(figure, target);
        return;
      case OutputFormat::Pdf:
        pdf_->save(figure, target);
        return;
      case OutputFormat::Png:
        raster_->save(figure, target);
        return;
      default:
        throw std::runtime_error("plotism::Figure2D::save: unsupported output format.");
    }
  }

  void save(const Figure3DView& figure, const RenderTarget& target) const override {
    switch (target.format) {
      case OutputFormat::Svg:
        svg_->save(figure, target);
        return;
      case OutputFormat::Pdf:
        pdf_->save(figure, target);
        return;
      case OutputFormat::Png:
        raster_->save(figure, target);
        return;
      default:
        throw std::runtime_error("plotism::Figure3D::save: unsupported output format.");
    }
  }

private:
  std::shared_ptr<const IBackend> svg_{};
  std::shared_ptr<const IBackend> pdf_{};
  std::shared_ptr<const IBackend> raster_{};
};

inline std::shared_ptr<const IBackend> make_default_backend() {
  static const auto backend = std::make_shared<const DefaultBackend>();
  return backend;
}

} // namespace pluss::plotism
