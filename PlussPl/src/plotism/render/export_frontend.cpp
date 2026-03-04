#include "plotism/render/export_bridge.h"

namespace pluss::pl {

std::string Studio::to_svg() const {
  return plotism::render::render_svg(*this);
}

std::string Studio::to_pdf() const {
  return plotism::render::render_pdf(*this);
}

void Studio::save_svg(std::string_view path) const {
  plotism::render::save_svg(*this, path);
}

void Studio::save_pdf(std::string_view path) const {
  plotism::render::save_pdf(*this, path);
}

} // namespace pluss::pl
