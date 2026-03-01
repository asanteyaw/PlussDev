#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "plot/modules/backend.h"

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

inline SubplotLayout2D make_subplot_layout(const Subplot2D& sub, const Rect2D& cell) {
  const bool has_y_label = !sub.y_axis.label.empty();
  const bool has_x_label = !sub.x_axis.label.empty();
  const double categorical_chars = estimate_max_label_chars(sub);
  const bool dense_categories = categorical_chars > 5.0;

  const double left_pad = has_y_label ? 72.0 : 60.0;
  const double right_pad = 18.0;
  const double top_pad = sub.title.empty() ? 24.0 : 32.0;
  const double bottom_pad = (has_x_label ? 14.0 : 4.0) + (dense_categories ? 34.0 : 24.0) + 16.0;

  const double plot_x = cell.x + left_pad;
  const double plot_y = cell.y + top_pad;
  const double plot_w = std::max(1.0, cell.width - left_pad - right_pad);
  const double plot_h = std::max(1.0, cell.height - top_pad - bottom_pad);

  return SubplotLayout2D{
      .cell = cell,
      .plot = Rect2D{.x = plot_x, .y = plot_y, .width = plot_w, .height = plot_h},
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
  const double cell_h = static_cast<double>(target.height - static_cast<int>(top_margin) - static_cast<int>(gap) * (fig.rows + 1)) /
                        static_cast<double>(std::max(1, fig.rows));

  FigureLayout2D layout{
      .canvas = Rect2D{.x = 0.0, .y = 0.0, .width = static_cast<double>(target.width), .height = static_cast<double>(target.height)},
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
