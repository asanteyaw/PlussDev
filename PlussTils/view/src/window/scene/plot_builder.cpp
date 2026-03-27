#include "pluss/tils/view/window/plot.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <numeric>
#include <ranges>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "pluss/tils/view/window/text_layout.h"

namespace pluss::view::window {

namespace {

constexpr double kMinWindowSpan = 1e-9;
constexpr int kMinAxisPixel = 2;

[[nodiscard]] constexpr bool valid_size(const SizeI size) noexcept {
  return size.width > 0 && size.height > 0;
}

[[nodiscard]] std::string format_tick(const double value) {
  const double abs_value = std::abs(value);
  int decimals = 0;
  if (abs_value < 10.0) {
    decimals = 2;
  } else if (abs_value < 100.0) {
    decimals = 1;
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(decimals) << value;
  return oss.str();
}

[[nodiscard]] std::pair<double, double> range_of(const std::span<const double> values) {
  if (values.empty()) {
    return {0.0, 1.0};
  }

  auto [min_it, max_it] = std::ranges::minmax_element(values);
  double min_value = *min_it;
  double max_value = *max_it;

  if (!std::isfinite(min_value) || !std::isfinite(max_value)) {
    return {0.0, 1.0};
  }

  if (std::abs(max_value - min_value) < 1e-12) {
    const double pad = std::max(1e-3, std::abs(max_value) * 0.05);
    min_value -= pad;
    max_value += pad;
  }
  return {min_value, max_value};
}

[[nodiscard]] double span_of(const std::pair<double, double>& range) {
  return range.second - range.first;
}

[[nodiscard]] constexpr int clamp_positive(const int value, const int fallback = 1) noexcept {
  return value > 0 ? value : fallback;
}

[[nodiscard]] constexpr int clamp_tick_count(const int value) noexcept {
  return std::max(2, value);
}

[[nodiscard]] constexpr int plot_left(const RectI rect) noexcept {
  return rect.x;
}

[[nodiscard]] constexpr int plot_top(const RectI rect) noexcept {
  return rect.y;
}

[[nodiscard]] constexpr int plot_right(const RectI rect) noexcept {
  return rect.x + rect.width - 1;
}

[[nodiscard]] constexpr int plot_bottom(const RectI rect) noexcept {
  return rect.y + rect.height - 1;
}

[[nodiscard]] PointI clamp_to_plot(const RectI rect, const PointI p) {
  return PointI{
      std::clamp(p.x, plot_left(rect), plot_right(rect)),
      std::clamp(p.y, plot_top(rect), plot_bottom(rect)),
  };
}

struct TickLabel {
  double t{0.0};
  double value{0.0};
  std::string text{};
  TextLayout layout{};
};

[[nodiscard]] std::vector<TickLabel> make_tick_labels(const std::pair<double, double>& range,
                                                      const int count,
                                                      const int pixel_size) {
  std::vector<TickLabel> labels(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(count - 1);
    const double value = range.first + t * (range.second - range.first);
    auto text = format_tick(value);
    labels[static_cast<std::size_t>(i)] =
        TickLabel{t, value, text, layout_math_text(text, pixel_size)};
  }
  return labels;
}

[[nodiscard]] int max_layout_height(const std::vector<TickLabel>& labels) {
  int max_height = 0;
  for (const auto& label : labels) {
    max_height = std::max(max_height, label.layout.ascent + label.layout.descent);
  }
  return max_height;
}

[[nodiscard]] int max_layout_width(const std::vector<TickLabel>& labels) {
  int max_width = 0;
  for (const auto& label : labels) {
    max_width = std::max(max_width, label.layout.width);
  }
  return max_width;
}

[[nodiscard]] RectI resolve_plot_rect(const LinePlotConfig& cfg,
                                      const std::pair<double, double>& x_range,
                                      const std::pair<double, double>& y_range) {
  const RectI fallback = cfg.plot_rect.empty() ? RectI{120, 90, 860, 520} : cfg.plot_rect;
  if (!cfg.fit_plot_to_viewport || !valid_size(cfg.viewport_size)) {
    return fallback;
  }

  const int x_ticks = clamp_tick_count(cfg.x_ticks);
  const int y_ticks = clamp_tick_count(cfg.y_ticks);
  const int tick_size = clamp_positive(cfg.tick_pixel_size, 12);
  const int label_size = clamp_positive(cfg.label_pixel_size, 14);
  const int title_size = clamp_positive(cfg.title_pixel_size, 18);
  const int tick_mark = clamp_positive(cfg.tick_mark_length, 6);
  const int tick_pad = clamp_positive(cfg.tick_label_padding, 8);
  const int axis_pad = clamp_positive(cfg.axis_label_padding, 18);
  const int title_pad = clamp_positive(cfg.title_padding, 14);

  const auto x_labels = make_tick_labels(x_range, x_ticks, tick_size);
  const auto y_labels = make_tick_labels(y_range, y_ticks, tick_size);
  const TextLayout x_label_layout = layout_math_text(cfg.x_label, label_size);
  const TextLayout y_label_layout = layout_math_text(cfg.y_label, label_size);
  const TextLayout title_layout = layout_math_text(cfg.title, title_size);

  const int x_tick_text_height = max_layout_height(x_labels);
  const int x_tick_text_width = max_layout_width(x_labels);
  const int y_tick_text_width = max_layout_width(y_labels);

  const int y_label_w = y_label_layout.width;
  const int title_h = title_layout.ascent + title_layout.descent;
  const int x_label_h = x_label_layout.ascent + x_label_layout.descent;

  const int left_margin = clamp_positive(cfg.viewport_pad_left, 0) + tick_mark + tick_pad +
                          y_tick_text_width + axis_pad + y_label_w;
  const int right_margin =
      clamp_positive(cfg.viewport_pad_right, 0) + std::max(tick_mark + tick_pad, x_tick_text_width / 2);
  const int top_margin = clamp_positive(cfg.viewport_pad_top, 0) + title_h + title_pad;
  const int bottom_margin =
      clamp_positive(cfg.viewport_pad_bottom, 0) + tick_mark + tick_pad + x_tick_text_height + axis_pad + x_label_h;

  const int min_w = clamp_positive(cfg.min_plot_width, 160);
  const int min_h = clamp_positive(cfg.min_plot_height, 120);
  const int max_w = std::max(min_w, cfg.viewport_size.width - 4);
  const int max_h = std::max(min_h, cfg.viewport_size.height - 4);

  const int target_w = std::clamp(cfg.viewport_size.width - left_margin - right_margin, min_w, max_w);
  const int target_h = std::clamp(cfg.viewport_size.height - top_margin - bottom_margin, min_h, max_h);

  int x = left_margin;
  int y = top_margin;
  if (x + target_w > cfg.viewport_size.width) {
    x = std::max(0, cfg.viewport_size.width - target_w);
  }
  if (y + target_h > cfg.viewport_size.height) {
    y = std::max(0, cfg.viewport_size.height - target_h);
  }

  return RectI{x, y, target_w, target_h};
}

[[nodiscard]] std::pair<double, double> clamp_window(const std::pair<double, double>& window,
                                                     const std::pair<double, double>& base) {
  const double base_span = std::max(kMinWindowSpan, span_of(base));
  double win_span = std::max(kMinWindowSpan, span_of(window));
  if (win_span >= base_span) {
    return base;
  }

  double min_value = window.first;
  double max_value = window.second;

  if (min_value < base.first) {
    max_value += (base.first - min_value);
    min_value = base.first;
  }
  if (max_value > base.second) {
    min_value -= (max_value - base.second);
    max_value = base.second;
  }

  if (min_value < base.first) {
    min_value = base.first;
  }
  if (max_value > base.second) {
    max_value = base.second;
  }

  if ((max_value - min_value) < kMinWindowSpan) {
    return base;
  }
  return {min_value, max_value};
}

[[nodiscard]] bool has_finite_window(const LinePlotViewWindow& window) {
  return window.active && std::isfinite(window.x_range.first) &&
         std::isfinite(window.x_range.second) && std::isfinite(window.y_range.first) &&
         std::isfinite(window.y_range.second) &&
         (window.x_range.second - window.x_range.first) > kMinWindowSpan &&
         (window.y_range.second - window.y_range.first) > kMinWindowSpan;
}

[[nodiscard]] std::tuple<std::pair<double, double>, std::pair<double, double>>
resolve_ranges(std::span<const double> x,
               std::span<const double> y,
               const LinePlotViewWindow& view_window) {
  const auto xr = range_of(x);
  const auto yr = range_of(y);
  if (!has_finite_window(view_window)) {
    return {xr, yr};
  }
  return {clamp_window(view_window.x_range, xr), clamp_window(view_window.y_range, yr)};
}

void add_frame(Scene& scene, const LinePlotConfig& cfg) {
  scene.add(ClearCommand{cfg.background});
  const RectI rect = cfg.plot_rect;
  const int axis_t = clamp_positive(cfg.axis_thickness, 1);

  if (cfg.show_plot_box) {
    scene.add(StrokeRectCommand{rect, axis_t, cfg.axis_color});
    return;
  }

  if (cfg.show_left_axis) {
    scene.add(LineCommand{
        PointI{plot_left(rect), plot_top(rect)},
        PointI{plot_left(rect), plot_bottom(rect)},
        axis_t,
        cfg.axis_color,
    });
  }
  if (cfg.show_bottom_axis) {
    scene.add(LineCommand{
        PointI{plot_left(rect), plot_bottom(rect)},
        PointI{plot_right(rect), plot_bottom(rect)},
        axis_t,
        cfg.axis_color,
    });
  }
  if (cfg.show_top_axis) {
    scene.add(LineCommand{
        PointI{plot_left(rect), plot_top(rect)},
        PointI{plot_right(rect), plot_top(rect)},
        axis_t,
        cfg.axis_color,
    });
  }
  if (cfg.show_right_axis) {
    scene.add(LineCommand{
        PointI{plot_right(rect), plot_top(rect)},
        PointI{plot_right(rect), plot_bottom(rect)},
        axis_t,
        cfg.axis_color,
    });
  }
}

void add_grid_and_ticks(Scene& scene,
                        const LinePlotConfig& cfg,
                        const std::pair<double, double>& x_range,
                        const std::pair<double, double>& y_range) {
  const RectI rect = cfg.plot_rect;
  const int x_ticks = clamp_tick_count(cfg.x_ticks);
  const int y_ticks = clamp_tick_count(cfg.y_ticks);
  const int tick_size = clamp_positive(cfg.tick_pixel_size, 12);
  const int label_size = clamp_positive(cfg.label_pixel_size, 14);
  const int title_size = clamp_positive(cfg.title_pixel_size, 18);
  const int tick_mark = clamp_positive(cfg.tick_mark_length, 6);
  const int tick_pad = clamp_positive(cfg.tick_label_padding, 8);
  const int axis_pad = clamp_positive(cfg.axis_label_padding, 18);
  const int title_pad = clamp_positive(cfg.title_padding, 14);
  const int grid_thickness = clamp_positive(cfg.grid_thickness, 1);
  const int axis_thickness = clamp_positive(cfg.axis_thickness, 2);

  const auto x_labels = make_tick_labels(x_range, x_ticks, tick_size);
  const auto y_labels = make_tick_labels(y_range, y_ticks, tick_size);
  const TextLayout x_label_layout = layout_math_text(cfg.x_label, label_size);
  const TextLayout y_label_layout = layout_math_text(cfg.y_label, label_size);
  const TextLayout title_layout = layout_math_text(cfg.title, title_size);

  const int x_tick_text_top = plot_bottom(rect) + tick_mark + tick_pad;
  const int x_tick_text_height = max_layout_height(x_labels);
  const int y_tick_text_width = max_layout_width(y_labels);

  for (const auto& label : x_labels) {
    const int x = plot_left(rect) +
                  static_cast<int>(std::round(label.t * static_cast<double>(std::max(kMinAxisPixel, rect.width - 1))));
    const bool at_left = std::abs(label.t) <= 1e-12;
    const bool at_right = std::abs(label.t - 1.0) <= 1e-12;
    if (cfg.grid_at_bounds || (!at_left && !at_right)) {
      scene.add(LineCommand{PointI{x, plot_top(rect)},
                            PointI{x, plot_bottom(rect)},
                            grid_thickness,
                            cfg.grid_color});
    }
    scene.add(LineCommand{PointI{x, plot_bottom(rect)},
                          PointI{x, plot_bottom(rect) + tick_mark},
                          axis_thickness,
                          cfg.axis_color});

    const int text_x = std::max(2, x - label.layout.width / 2);
    const int text_baseline = x_tick_text_top + label.layout.ascent;
    scene.add(TextCommand{PointI{text_x, text_baseline}, label.text, tick_size, cfg.axis_color});
  }

  for (const auto& label : y_labels) {
    const int y = plot_bottom(rect) -
                  static_cast<int>(std::round(label.t * static_cast<double>(std::max(kMinAxisPixel, rect.height - 1))));
    const bool at_bottom = std::abs(label.t) <= 1e-12;
    const bool at_top = std::abs(label.t - 1.0) <= 1e-12;
    if (cfg.grid_at_bounds || (!at_bottom && !at_top)) {
      scene.add(LineCommand{PointI{plot_left(rect), y},
                            PointI{plot_right(rect), y},
                            grid_thickness,
                            cfg.grid_color});
    }
    scene.add(LineCommand{PointI{plot_left(rect) - tick_mark, y},
                          PointI{plot_left(rect), y},
                          axis_thickness,
                          cfg.axis_color});

    const int text_x = std::max(2, plot_left(rect) - tick_mark - tick_pad - label.layout.width);
    const int text_top = y - (label.layout.ascent + label.layout.descent) / 2;
    scene.add(TextCommand{PointI{text_x, text_top + label.layout.ascent}, label.text, tick_size, cfg.axis_color});
  }

  const int x_label_x = std::max(2, plot_left(rect) + (rect.width - x_label_layout.width) / 2);
  const int x_label_top = x_tick_text_top + x_tick_text_height + axis_pad;
  scene.add(TextCommand{PointI{x_label_x, x_label_top + x_label_layout.ascent},
                        cfg.x_label,
                        label_size,
                        cfg.axis_color});

  const int y_label_x =
      std::max(2, plot_left(rect) - tick_mark - tick_pad - y_tick_text_width - axis_pad - y_label_layout.width);
  const int y_label_top = plot_top(rect) + (rect.height - (y_label_layout.ascent + y_label_layout.descent)) / 2;
  scene.add(TextCommand{PointI{y_label_x, y_label_top + y_label_layout.ascent},
                        cfg.y_label,
                        label_size,
                        cfg.axis_color});

  const int title_x = std::max(2, plot_left(rect) + (rect.width - title_layout.width) / 2);
  const int title_baseline = std::max(2, plot_top(rect) - title_pad);
  scene.add(TextCommand{PointI{title_x, title_baseline}, cfg.title, title_size, cfg.axis_color});
}

[[nodiscard]] std::vector<PointI> map_line_points(std::span<const double> x,
                                                  std::span<const double> y,
                                                  const LinePlotConfig& cfg,
                                                  const RectI rect,
                                                  const std::pair<double, double>& x_range,
                                                  const std::pair<double, double>& y_range) {
  std::vector<PointI> points;
  const std::size_t n = std::min(x.size(), y.size());
  points.reserve(n);

  const double xr = x_range.second - x_range.first;
  const double yr = y_range.second - y_range.first;
  if (xr <= kMinWindowSpan || yr <= kMinWindowSpan) {
    return points;
  }

  for (std::size_t i = 0; i < n; ++i) {
    if (!std::isfinite(x[i]) || !std::isfinite(y[i])) {
      continue;
    }

    const double nx = (x[i] - x_range.first) / xr;
    const double ny = (y[i] - y_range.first) / yr;

    const int px = rect.x +
                   static_cast<int>(std::round(nx * static_cast<double>(std::max(kMinAxisPixel, rect.width - 1))));
    const int py = rect.y + rect.height - 1 -
                   static_cast<int>(std::round(ny * static_cast<double>(std::max(kMinAxisPixel, rect.height - 1))));
    PointI point{px, py};
    if (cfg.clamp_points_to_plot) {
      point = clamp_to_plot(rect, point);
    }
    points.push_back(point);
  }

  return points;
}

} // namespace

Scene build_line_plot_scene(std::span<const double> y, const LinePlotConfig& config) {
  std::vector<double> x(y.size(), 0.0);
  std::iota(x.begin(), x.end(), 0.0);
  return build_line_plot_scene(std::span<const double>(x.data(), x.size()), y, config);
}

Scene build_line_plot_scene(std::span<const double> x,
                            std::span<const double> y,
                            const LinePlotConfig& config) {
  return build_line_plot_scene(x, y, config, LinePlotViewWindow{});
}

Scene build_line_plot_scene(std::span<const double> x,
                            std::span<const double> y,
                            const LinePlotConfig& config,
                            const LinePlotViewWindow& view_window) {
  Scene scene;

  const auto [xr, yr] = resolve_ranges(x, y, view_window);
  LinePlotConfig resolved = config;
  resolved.plot_rect = resolve_plot_rect(config, xr, yr);

  add_frame(scene, resolved);

  add_grid_and_ticks(scene, resolved, xr, yr);
  const auto points = map_line_points(x, y, resolved, resolved.plot_rect, xr, yr);

  scene.add(SetClipRectCommand{resolved.plot_rect});
  scene.add(PolylineCommand{points, clamp_positive(resolved.line_thickness, 2), resolved.line_color});
  if (resolved.markers) {
    scene.add(MarkerCloudCommand{points, clamp_positive(resolved.marker_radius, 2), resolved.marker_color});
  }
  scene.add(ClearClipRectCommand{});

  return scene;
}

LinePlotController::LinePlotController(std::span<const double> y, LinePlotConfig config)
    : config_(std::move(config)) {
  x_.assign(y.size(), 0.0);
  std::iota(x_.begin(), x_.end(), 0.0);
  y_.assign(y.begin(), y.end());

  base_window_.x_range = range_of(std::span<const double>(x_.data(), x_.size()));
  base_window_.y_range = range_of(std::span<const double>(y_.data(), y_.size()));
  base_window_.active = true;
  active_window_ = base_window_;
}

LinePlotController::LinePlotController(std::span<const double> x,
                                       std::span<const double> y,
                                       LinePlotConfig config)
    : config_(std::move(config)) {
  const auto n = std::min(x.size(), y.size());
  x_.assign(x.begin(), x.begin() + static_cast<std::ptrdiff_t>(n));
  y_.assign(y.begin(), y.begin() + static_cast<std::ptrdiff_t>(n));

  base_window_.x_range = range_of(std::span<const double>(x_.data(), x_.size()));
  base_window_.y_range = range_of(std::span<const double>(y_.data(), y_.size()));
  base_window_.active = true;
  active_window_ = base_window_;
}

bool LinePlotController::zoom_(const double factor, const double anchor_x, const double anchor_y) {
  if (!std::isfinite(factor) || factor <= 0.0) {
    return false;
  }

  const double ax = std::clamp(anchor_x, 0.0, 1.0);
  const double ay = std::clamp(anchor_y, 0.0, 1.0);

  const auto [x_min, x_max] = active_window_.x_range;
  const auto [y_min, y_max] = active_window_.y_range;
  const double x_span = std::max(kMinWindowSpan, x_max - x_min);
  const double y_span = std::max(kMinWindowSpan, y_max - y_min);

  const double next_x_span = std::clamp(x_span / factor, kMinWindowSpan, span_of(base_window_.x_range));
  const double next_y_span = std::clamp(y_span / factor, kMinWindowSpan, span_of(base_window_.y_range));

  const double x_anchor = x_min + ax * x_span;
  const double y_anchor = y_min + ay * y_span;

  const std::pair<double, double> next_x{x_anchor - ax * next_x_span,
                                         x_anchor + (1.0 - ax) * next_x_span};
  const std::pair<double, double> next_y{y_anchor - ay * next_y_span,
                                         y_anchor + (1.0 - ay) * next_y_span};

  active_window_.x_range = clamp_window(next_x, base_window_.x_range);
  active_window_.y_range = clamp_window(next_y, base_window_.y_range);
  active_window_.active = true;
  return true;
}

bool LinePlotController::pan_(const double dx_ratio, const double dy_ratio) {
  if (!std::isfinite(dx_ratio) || !std::isfinite(dy_ratio)) {
    return false;
  }
  const auto [x_min, x_max] = active_window_.x_range;
  const auto [y_min, y_max] = active_window_.y_range;

  const double x_span = std::max(kMinWindowSpan, x_max - x_min);
  const double y_span = std::max(kMinWindowSpan, y_max - y_min);

  const double dx = dx_ratio * x_span;
  const double dy = dy_ratio * y_span;

  active_window_.x_range = clamp_window({x_min + dx, x_max + dx}, base_window_.x_range);
  active_window_.y_range = clamp_window({y_min + dy, y_max + dy}, base_window_.y_range);
  active_window_.active = true;
  return true;
}

bool LinePlotController::apply(const LinePlotInteraction& interaction) {
  switch (interaction.kind) {
    case LinePlotInteractionKind::Pan:
      return pan_(interaction.dx_ratio, interaction.dy_ratio);
    case LinePlotInteractionKind::Zoom:
      return zoom_(interaction.zoom_factor, interaction.anchor_x, interaction.anchor_y);
    case LinePlotInteractionKind::Reset:
      reset_view();
      return true;
  }
  return false;
}

void LinePlotController::reset_view() noexcept {
  active_window_ = base_window_;
}

Scene LinePlotController::build_scene() const {
  return build_line_plot_scene(std::span<const double>(x_.data(), x_.size()),
                               std::span<const double>(y_.data(), y_.size()),
                               config_,
                               active_window_);
}

LinePlotViewWindow LinePlotController::base_view_window() const noexcept {
  return base_window_;
}

LinePlotViewWindow LinePlotController::active_view_window() const noexcept {
  return active_window_;
}

void LinePlotController::set_viewport_size(const SizeI size) noexcept {
  if (!valid_size(size)) {
    return;
  }
  config_.viewport_size = size;
}

SizeI LinePlotController::viewport_size() const noexcept {
  return config_.viewport_size;
}

} // namespace pluss::view::window
