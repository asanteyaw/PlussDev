#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "pluss/view/window/scene.h"

namespace pluss::view::window {

struct LinePlotConfig {
  RectI plot_rect{120, 90, 860, 520};
  std::string title{"Line Plot"};
  std::string x_label{"x"};
  std::string y_label{"y"};
  Color background{Color::rgba(248, 248, 248, 255)};
  Color axis_color{Color::rgba(28, 28, 28, 255)};
  Color grid_color{Color::rgba(210, 214, 220, 255)};
  Color line_color{Color::rgba(24, 114, 220, 255)};
  Color marker_color{Color::rgba(220, 72, 68, 255)};
  int x_ticks{6};
  int y_ticks{6};
  int axis_thickness{2};
  int grid_thickness{1};
  int line_thickness{2};
  int marker_radius{2};
  int title_pixel_size{18};
  int label_pixel_size{14};
  int tick_pixel_size{12};
  int tick_mark_length{6};
  int tick_label_padding{8};
  int axis_label_padding{18};
  int title_padding{14};
  bool show_plot_box{false};
  bool show_left_axis{true};
  bool show_bottom_axis{true};
  bool show_top_axis{false};
  bool show_right_axis{false};
  bool grid_at_bounds{false};
  bool fit_plot_to_viewport{true};
  SizeI viewport_size{};
  int viewport_pad_left{24};
  int viewport_pad_right{24};
  int viewport_pad_top{24};
  int viewport_pad_bottom{24};
  int min_plot_width{240};
  int min_plot_height{180};
  bool clamp_points_to_plot{false};
  bool markers{false};
};

struct LinePlotViewWindow {
  std::pair<double, double> x_range{0.0, 1.0};
  std::pair<double, double> y_range{0.0, 1.0};
  bool active{false};
};

enum class LinePlotInteractionKind : std::uint8_t {
  Pan = 0,
  Zoom = 1,
  Reset = 2,
};

struct LinePlotInteraction {
  LinePlotInteractionKind kind{LinePlotInteractionKind::Reset};
  double dx_ratio{0.0};      // positive moves view right in data-space
  double dy_ratio{0.0};      // positive moves view up in data-space
  double zoom_factor{1.0};   // >1 zoom in, (0,1) zoom out
  double anchor_x{0.5};      // [0,1] in plot viewport
  double anchor_y{0.5};      // [0,1] in plot viewport
};

[[nodiscard]] Scene build_line_plot_scene(std::span<const double> y, const LinePlotConfig& config = {});
[[nodiscard]] Scene build_line_plot_scene(std::span<const double> x,
                                          std::span<const double> y,
                                          const LinePlotConfig& config = {});
[[nodiscard]] Scene build_line_plot_scene(std::span<const double> x,
                                          std::span<const double> y,
                                          const LinePlotConfig& config,
                                          const LinePlotViewWindow& view_window);

class LinePlotController {
 public:
  explicit LinePlotController(std::span<const double> y, LinePlotConfig config = {});
  LinePlotController(std::span<const double> x,
                     std::span<const double> y,
                     LinePlotConfig config = {});

  [[nodiscard]] bool apply(const LinePlotInteraction& interaction);
  void reset_view() noexcept;

  [[nodiscard]] Scene build_scene() const;
  [[nodiscard]] LinePlotViewWindow base_view_window() const noexcept;
  [[nodiscard]] LinePlotViewWindow active_view_window() const noexcept;
  void set_viewport_size(SizeI size) noexcept;
  [[nodiscard]] SizeI viewport_size() const noexcept;

 private:
  [[nodiscard]] bool zoom_(double factor, double anchor_x, double anchor_y);
  [[nodiscard]] bool pan_(double dx_ratio, double dy_ratio);

  std::vector<double> x_{};
  std::vector<double> y_{};
  LinePlotConfig config_{};
  LinePlotViewWindow base_window_{};
  LinePlotViewWindow active_window_{};
};

} // namespace pluss::view::window
