#include "pluss/pl/plusspl.h"

#include <algorithm>
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
  default_legend_place_ = pane.legend_place();
  default_legend_frame_ = pane.legend_frame();
  default_legend_shape_ = pane.legend_shape();
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

Studio& Studio::legend_shape(KeyShape shape) noexcept {
  legend_shape_ = shape;
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

Studio& Studio::row_title(std::int64_t row, std::string text) {
  if (row < 0) {
    throw std::invalid_argument("row_title: row must be non-negative");
  }
  row_titles_[row] = std::move(text);
  return *this;
}

Studio& Studio::clear_row_title(std::int64_t row) noexcept {
  row_titles_.erase(row);
  return *this;
}

Studio& Studio::clear_row_titles() noexcept {
  row_titles_.clear();
  return *this;
}

Pane& Studio::pane2(std::string title) {
  panes_.emplace_back(PaneKind::Planar, theme_, std::move(title));
  auto& pane = panes_.back();
  pane.tick_font_scale(default_tick_font_scale_).axis_label_font_scale(default_axis_label_font_scale_);
  if (pane_style_defaults_enabled_) {
    pane.theme_tokens() = default_pane_theme_;
    pane.grid(default_grid_style_)
        .legend(default_legend_place_)
        .legend_frame(default_legend_frame_)
        .legend_shape(default_legend_shape_)
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
        .legend(default_legend_place_)
        .legend_frame(default_legend_frame_)
        .legend_shape(default_legend_shape_)
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

std::size_t Studio::pane_index(const Pane& pane) const {
  const auto* target = &pane;
  const auto it = std::find_if(panes_.begin(), panes_.end(), [&](const Pane& candidate) {
    return &candidate == target;
  });
  if (it == panes_.end()) {
    throw std::invalid_argument("pane_index: pane does not belong to this studio");
  }
  return static_cast<std::size_t>(std::distance(panes_.begin(), it));
}

Pane& Studio::subpane2(
    std::size_t parent_index,
    std::int64_t rows,
    std::int64_t cols,
    std::int64_t row,
    std::int64_t col,
    std::int64_t row_span,
    std::int64_t col_span,
    std::string title) {
  if (parent_index >= panes_.size()) {
    throw std::out_of_range("subpane2: parent_index out of range");
  }
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument("subpane2: rows and cols must be positive");
  }
  if (row < 0 || col < 0 || row_span <= 0 || col_span <= 0) {
    throw std::invalid_argument("subpane2: row/col must be non-negative and spans must be positive");
  }
  if (row + row_span > rows || col + col_span > cols) {
    throw std::invalid_argument("subpane2: requested slot exceeds local subgrid bounds");
  }
  Pane& pane = pane2(std::move(title));
  pane.placement_ = PanePlacement{
      row,
      col,
      row_span,
      col_span,
      true,
      static_cast<std::int64_t>(parent_index),
      rows,
      cols,
      12.0,
      12.0,
  };
  return pane;
}

Pane& Studio::subpane3(
    std::size_t parent_index,
    std::int64_t rows,
    std::int64_t cols,
    std::int64_t row,
    std::int64_t col,
    std::int64_t row_span,
    std::int64_t col_span,
    std::string title) {
  if (parent_index >= panes_.size()) {
    throw std::out_of_range("subpane3: parent_index out of range");
  }
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument("subpane3: rows and cols must be positive");
  }
  if (row < 0 || col < 0 || row_span <= 0 || col_span <= 0) {
    throw std::invalid_argument("subpane3: row/col must be non-negative and spans must be positive");
  }
  if (row + row_span > rows || col + col_span > cols) {
    throw std::invalid_argument("subpane3: requested slot exceeds local subgrid bounds");
  }
  Pane& pane = pane3(std::move(title));
  pane.placement_ = PanePlacement{
      row,
      col,
      row_span,
      col_span,
      true,
      static_cast<std::int64_t>(parent_index),
      rows,
      cols,
      12.0,
      12.0,
  };
  return pane;
}

Studio studio(std::string name) {
  return Studio(std::move(name));
}

} // namespace pluss::pl
