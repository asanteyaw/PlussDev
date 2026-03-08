#include "pluss/pl/plusspl.h"

#include <cmath>
#include <stdexcept>

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

Studio& Studio::context_scales(double tick_font_scale, double axis_label_font_scale) {
  if (!(tick_font_scale > 0.0 && std::isfinite(tick_font_scale))) {
    throw std::invalid_argument("context_scales: tick_font_scale must be finite and positive");
  }
  if (!(axis_label_font_scale > 0.0 && std::isfinite(axis_label_font_scale))) {
    throw std::invalid_argument("context_scales: axis_label_font_scale must be finite and positive");
  }
  default_tick_font_scale_ = tick_font_scale;
  default_axis_label_font_scale_ = axis_label_font_scale;
  return *this;
}

Studio& Studio::pane_style_defaults(const Pane& pane) {
  pane_style_defaults_enabled_ = true;
  default_pane_theme_ = pane.theme_tokens();
  default_axes_visible_ = pane.axes_visible();
  default_grid_style_ = pane.grid_style();
  default_x_major_grid_ = pane.x_axis().major_grid;
  default_y_major_grid_ = pane.y_axis().major_grid;
  default_z_major_grid_ = pane.z_axis().major_grid;
  default_x_minor_grid_ = pane.x_axis().minor_grid;
  default_y_minor_grid_ = pane.y_axis().minor_grid;
  default_z_minor_grid_ = pane.z_axis().minor_grid;
  return *this;
}

Studio& Studio::clear_pane_style_defaults() noexcept {
  pane_style_defaults_enabled_ = false;
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

Studio& Studio::tight_layout(bool enable, double padding) {
  if (!(padding >= 0.0 && std::isfinite(padding))) {
    throw std::invalid_argument("tight_layout: padding must be finite and non-negative");
  }
  tight_layout_enabled_ = enable;
  tight_layout_padding_ = padding;
  return *this;
}

Studio& Studio::legend(KeyDock dock) noexcept {
  legend_dock_ = dock;
  return *this;
}

Studio& Studio::legend_frame(KeyFrame frame) noexcept {
  legend_frame_ = frame;
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
  auto& pane = panes_.back();
  pane.tick_font_scale(default_tick_font_scale_).axis_label_font_scale(default_axis_label_font_scale_);
  if (pane_style_defaults_enabled_) {
    pane.theme_tokens() = default_pane_theme_;
    pane.grid(default_grid_style_)
        .x_major_grid(default_x_major_grid_)
        .y_major_grid(default_y_major_grid_)
        .z_major_grid(default_z_major_grid_)
        .x_minor_grid(default_x_minor_grid_)
        .y_minor_grid(default_y_minor_grid_)
        .z_minor_grid(default_z_minor_grid_)
        .axes_visible(default_axes_visible_);
  }
  return panes_.back();
}

Pane& Studio::pane3(std::string title) {
  panes_.emplace_back(PaneKind::Volumetric, theme_, std::move(title));
  auto& pane = panes_.back();
  pane.tick_font_scale(default_tick_font_scale_).axis_label_font_scale(default_axis_label_font_scale_);
  if (pane_style_defaults_enabled_) {
    pane.theme_tokens() = default_pane_theme_;
    pane.grid(default_grid_style_)
        .x_major_grid(default_x_major_grid_)
        .y_major_grid(default_y_major_grid_)
        .z_major_grid(default_z_major_grid_)
        .x_minor_grid(default_x_minor_grid_)
        .y_minor_grid(default_y_minor_grid_)
        .z_minor_grid(default_z_minor_grid_)
        .axes_visible(default_axes_visible_);
  }
  return panes_.back();
}

Studio studio(std::string name) {
  return Studio(std::move(name));
}

} // namespace pluss::pl
