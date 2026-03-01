#pragma once

#include <memory>
#include <stdexcept>

#include "plot/modules/backend.h"
#include "plot/modules/backend/raster/render_2d.h"
#include "plot/modules/backend/raster/render_3d.h"

namespace pluss::plotism {

class RasterBackend final : public IBackend {
public:
  void save(const Figure2DView& figure, const RenderTarget& target) const override {
    if (target.format != OutputFormat::Png) {
      throw std::runtime_error("plotism::RasterBackend only supports save_png().");
    }
    detail::write_text_file(target.path, detail::render_png_2d(figure, target));
  }

  void save(const Figure3DView& figure, const RenderTarget& target) const override {
    if (target.format != OutputFormat::Png) {
      throw std::runtime_error("plotism::RasterBackend only supports save_png().");
    }
    detail::write_text_file(target.path, detail::render_png_3d(figure, target));
  }
};

inline std::shared_ptr<const IBackend> make_raster_backend() {
  static const auto backend = std::make_shared<const RasterBackend>();
  return backend;
}

} // namespace pluss::plotism
