#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "plot/modules/backend.h"
#include "plot/modules/backend/render_surface_2d.h"

namespace pluss::plotism {
namespace detail {

struct Rect2D {
  double x{0.0};
  double y{0.0};
  double width{0.0};
  double height{0.0};
};

struct SubplotLayout2D {
  Rect2D cell{};
  Rect2D plot{};
  std::optional<Rect2D> legend_box{};
  std::optional<Rect2D> colorbar_box{};
  double title_y{0.0};
  double x_label_y{0.0};
  double y_label_x{0.0};
};

struct FigureLayout2D {
  Rect2D canvas{};
  double figure_title_y{0.0};
  std::vector<SubplotLayout2D> subplots{};
};

inline double estimate_max_label_chars(const Subplot2D& sub) {
  double longest = 0.0;
  for (const auto& trace : sub.traces) {
    for (const auto& label : trace.x_labels) {
      longest = std::max(longest, static_cast<double>(label.size()));
    }
  }
  return longest;
}

inline bool has_heatmap_trace(const Subplot2D& sub) {
  return std::any_of(sub.traces.begin(), sub.traces.end(), [](const Trace2D& trace) {
    return trace.kind == Trace2DKind::Heatmap && trace.rows > 0 && trace.cols > 0;
  });
}

inline double legend_width_hint_for_subplot(const Subplot2D& sub) {
  std::size_t longest = sub.legend.title.size();
  for (const auto& trace : sub.traces) {
    longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
  }
  return legend_width_hint_2d(sub.traces.size(), longest, !sub.legend.title.empty());
}

inline double legend_height_hint_for_subplot(const Subplot2D& sub) {
  return legend_height_hint_2d(sub.traces.size(), !sub.legend.title.empty());
}

inline SubplotLayout2D make_subplot_layout(const Subplot2D& sub, const Rect2D& cell) {
  const bool has_y_label = !sub.y_axis.label.empty();
  const bool has_x_label = !sub.x_axis.label.empty();
  const double categorical_chars = estimate_max_label_chars(sub);
  const bool dense_categories = categorical_chars > 5.0;
  const bool outside_legend =
      sub.legend.visible && !sub.traces.empty() && sub.legend.location == LegendLocation::OutsideRight;
  const bool show_colorbar =
      sub.colorbar.has_value() && sub.colorbar->visible && sub.colorbar->location == ColorbarLocation::Right &&
      has_heatmap_trace(sub);

  const double left_pad = has_y_label ? 72.0 : 60.0;
  const double base_right_pad = 18.0;
  const double top_pad = sub.title.empty() ? 24.0 : 32.0;
  const double bottom_pad = (has_x_label ? 14.0 : 4.0) + (dense_categories ? 34.0 : 24.0) + 16.0;

  const double colorbar_slot_width = show_colorbar ? 58.0 : 0.0;
  const double legend_slot_width = outside_legend ? legend_width_hint_for_subplot(sub) : 0.0;
  const double slot_gap = (show_colorbar && outside_legend) ? 12.0 : 0.0;
  const double right_pad = base_right_pad + colorbar_slot_width + legend_slot_width + slot_gap;

  const double plot_x = cell.x + left_pad;
  const double plot_y = cell.y + top_pad;
  const double plot_w = std::max(1.0, cell.width - left_pad - right_pad);
  const double plot_h = std::max(1.0, cell.height - top_pad - bottom_pad);

  std::optional<Rect2D> colorbar_box;
  std::optional<Rect2D> legend_box;
  double aux_x = plot_x + plot_w + 12.0;
  if (show_colorbar) {
    colorbar_box = Rect2D{
        .x = aux_x,
        .y = plot_y + 6.0,
        .width = 52.0,
        .height = std::max(48.0, plot_h - 12.0),
    };
    aux_x += colorbar_box->width + slot_gap;
  }
  if (outside_legend) {
    const double legend_height = legend_height_hint_for_subplot(sub) + 20.0;
    legend_box = Rect2D{
        .x = aux_x,
        .y = plot_y,
        .width = legend_slot_width,
        .height = std::min(std::max(44.0, legend_height), std::max(44.0, cell.height - top_pad - 10.0)),
    };
  }

  return SubplotLayout2D{
      .cell = cell,
      .plot = Rect2D{.x = plot_x, .y = plot_y, .width = plot_w, .height = plot_h},
      .legend_box = legend_box,
      .colorbar_box = colorbar_box,
      .title_y = cell.y + 20.0,
      .x_label_y = cell.y + cell.height - 14.0,
      .y_label_x = cell.x + 18.0,
  };
}

inline FigureLayout2D make_figure_layout_2d(const Figure2DView& fig, const RenderTarget& target) {
  const double top_margin = fig.title.empty() ? 24.0 : 52.0;
  const double gap = 20.0;
  const double cell_w = static_cast<double>(target.width - static_cast<int>(gap) * (fig.cols + 1)) /
                        static_cast<double>(std::max(1, fig.cols));
  const double cell_h =
      static_cast<double>(target.height - static_cast<int>(top_margin) - static_cast<int>(gap) * (fig.rows + 1)) /
      static_cast<double>(std::max(1, fig.rows));

  FigureLayout2D layout{
      .canvas =
          Rect2D{.x = 0.0, .y = 0.0, .width = static_cast<double>(target.width), .height = static_cast<double>(target.height)},
      .figure_title_y = 28.0,
      .subplots = {},
  };
  layout.subplots.reserve(fig.subplots.size());

  for (int r = 0; r < fig.rows; ++r) {
    for (int c = 0; c < fig.cols; ++c) {
      const int idx = r * fig.cols + c;
      const auto& sub = fig.subplots[static_cast<std::size_t>(idx)];
      const Rect2D cell{
          .x = gap + static_cast<double>(c) * (cell_w + gap),
          .y = top_margin + gap + static_cast<double>(r) * (cell_h + gap),
          .width = cell_w,
          .height = cell_h,
      };
      layout.subplots.push_back(make_subplot_layout(sub, cell));
    }
  }

  return layout;
}

} // namespace detail
} // namespace pluss::plotism
