#include "pluss/pl/plusspl.h"

#include <cmath>

namespace pluss::pl {

Studio::Studio(std::string name, Theme theme)
    : name_(std::move(name)), theme_(std::move(theme)) {}

Studio& Studio::rename(std::string name) {
  name_ = std::move(name);
  return *this;
}

Studio& Studio::headline(std::string text) {
  headline_ = std::move(text);
  return *this;
}

Studio& Studio::theme(Theme theme) {
  theme_ = std::move(theme);
  return *this;
}

Studio& Studio::size(double width, double height) noexcept {
  extent_.width = width;
  extent_.height = height;
  return *this;
}

Studio& Studio::mosaic(std::int64_t rows, std::int64_t cols, double gutter_x, double gutter_y) {
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument("mosaic: rows and cols must be positive");
  }
  if (!(gutter_x >= 0.0 && gutter_y >= 0.0 && std::isfinite(gutter_x) && std::isfinite(gutter_y))) {
    throw std::invalid_argument("mosaic: gutters must be finite and non-negative");
  }
  mosaic_layout_ = MosaicLayout{rows, cols, gutter_x, gutter_y, true};
  return *this;
}

Studio& Studio::trace_key(KeyDock dock) noexcept {
  trace_key_dock_ = dock;
  return *this;
}

Studio& Studio::x_caption(std::string group, std::string text) {
  x_captions_[std::move(group)] = std::move(text);
  return *this;
}

Studio& Studio::y_caption(std::string group, std::string text) {
  y_captions_[std::move(group)] = std::move(text);
  return *this;
}

Studio& Studio::z_caption(std::string group, std::string text) {
  z_captions_[std::move(group)] = std::move(text);
  return *this;
}

Pane& Studio::pane2(std::string title) {
  panes_.emplace_back(PaneKind::Planar, theme_, std::move(title));
  return panes_.back();
}

Pane& Studio::pane3(std::string title) {
  panes_.emplace_back(PaneKind::Volumetric, theme_, std::move(title));
  return panes_.back();
}

Studio studio(std::string name) {
  return Studio(std::move(name));
}

} // namespace pluss::pl
