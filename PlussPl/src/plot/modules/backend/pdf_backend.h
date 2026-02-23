#pragma once

#include <memory>
#include <stdexcept>

#include "plot/modules/backend.h"
#include "plot/modules/backend/pdf/render_2d.h"
#include "plot/modules/backend/pdf/render_3d.h"

namespace pluss::plotism {

class PdfBackend final : public IBackend {
public:
  void save(const Figure2DView& figure, const RenderTarget& target) const override {
    if (target.format != OutputFormat::Pdf) {
      throw std::runtime_error("plotism::PdfBackend only supports save_pdf().");
    }
    detail::write_text_file(target.path, detail::render_pdf_2d(figure, target));
  }

  void save(const Figure3DView& figure, const RenderTarget& target) const override {
    if (target.format != OutputFormat::Pdf) {
      throw std::runtime_error("plotism::PdfBackend only supports save_pdf().");
    }
    detail::write_text_file(target.path, detail::render_pdf_3d(figure, target));
  }
};

inline std::shared_ptr<const IBackend> make_pdf_backend() {
  static const auto backend = std::make_shared<const PdfBackend>();
  return backend;
}

} // namespace pluss::plotism
