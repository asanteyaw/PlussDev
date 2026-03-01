#pragma once

#include <stdexcept>

#include "plot/modules/artists/artist_2d.h"
#include "plot/modules/backend.h"
#include "plot/modules/backend/layout_2d.h"
#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

inline void render_2d_scene(RenderSurface2D& surface, const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save: width/height too small");
  }

  surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                    FillStyle2D{.color = "#f7f8fb", .alpha = 1.0});

  const auto layout = make_figure_layout_2d(fig, target);

  if (!fig.title.empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, layout.figure_title_y, fig.title,
                 TextStyle2D{
                     .color = "#111827",
                     .size = 22.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                 });
  }

  const auto& palette = detail::default_palette();
  for (int r = 0; r < fig.rows; ++r) {
    for (int c = 0; c < fig.cols; ++c) {
      const int idx = r * fig.cols + c;
      const auto& sub = fig.subplots[static_cast<std::size_t>(idx)];
      const auto& subplot_layout = layout.subplots[static_cast<std::size_t>(idx)];
      const auto& cell = subplot_layout.cell;
      const auto& plot = subplot_layout.plot;
      const double left = plot.x;
      const double right = plot.x + plot.width;
      const double top = plot.y;
      const double bottom = plot.y + plot.height;
      const double plot_w = plot.width;
      const double plot_h = plot.height;

      surface.fill_rect(cell.x, cell.y, cell.width, cell.height, FillStyle2D{.color = "white", .alpha = 1.0}, 12.0);
      surface.stroke_rect(cell.x, cell.y, cell.width, cell.height,
                          StrokeStyle2D{.color = "#d9dfeb", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"},
                          12.0);

      surface.fill_rect(plot.x, plot.y, plot.width, plot.height, FillStyle2D{.color = "#fcfcfe", .alpha = 1.0});
      surface.stroke_rect(plot.x, plot.y, plot.width, plot.height,
                          StrokeStyle2D{.color = "#eef1f7", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});

      if (!sub.title.empty()) {
        surface.text(cell.x + cell.width * 0.5, subplot_layout.title_y, sub.title,
                     TextStyle2D{
                         .color = "#111827",
                         .size = 14.0,
                         .alpha = 1.0,
                         .bold = true,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::Horizontal,
                     });
      }
      if (!sub.x_axis.label.empty()) {
        surface.text(cell.x + cell.width * 0.5, subplot_layout.x_label_y, sub.x_axis.label,
                     TextStyle2D{
                         .color = "#2c3648",
                         .size = 12.0,
                         .alpha = 1.0,
                         .bold = false,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::Horizontal,
                     });
      }
      if (!sub.y_axis.label.empty()) {
        surface.text(subplot_layout.y_label_x, cell.y + cell.height * 0.5, sub.y_axis.label,
                     TextStyle2D{
                         .color = "#2c3648",
                         .size = 12.0,
                         .alpha = 1.0,
                         .bold = false,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::VerticalCcw,
                     });
      }

      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();
      for (const auto& t : sub.traces) {
        const std::size_t n = std::min(t.x.size(), t.y.size());
        for (std::size_t i = 0; i < n; ++i) {
          const double tx = axis_transform_2d(t.x[i], sub.x_axis);
          const double ty = axis_transform_2d(t.y[i], sub.y_axis);
          if (std::isfinite(tx)) {
            xmin = std::min(xmin, tx);
            xmax = std::max(xmax, tx);
          }
          if (std::isfinite(ty)) {
            ymin = std::min(ymin, ty);
            ymax = std::max(ymax, ty);
          }
        }
        if ((t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) && sub.y_axis.scale == AxisScale::Linear) {
          ymin = std::min(ymin, 0.0);
        }
      }

      if (sub.x_axis.min.has_value()) xmin = axis_transform_2d(*sub.x_axis.min, sub.x_axis);
      if (sub.x_axis.max.has_value()) xmax = axis_transform_2d(*sub.x_axis.max, sub.x_axis);
      if (sub.y_axis.min.has_value()) ymin = axis_transform_2d(*sub.y_axis.min, sub.y_axis);
      if (sub.y_axis.max.has_value()) ymax = axis_transform_2d(*sub.y_axis.max, sub.y_axis);

      std::tie(xmin, xmax) = ensure_limits_2d(xmin, xmax);
      std::tie(ymin, ymax) = ensure_limits_2d(ymin, ymax);

      const auto sx = [&](double x) {
        const double tx = axis_transform_2d(x, sub.x_axis);
        if (!std::isfinite(tx)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double xn = (tx - xmin) / (xmax - xmin);
        if (sub.x_axis.inverted) {
          xn = 1.0 - xn;
        }
        return left + xn * plot_w;
      };
      const auto sy = [&](double y) {
        const double ty = axis_transform_2d(y, sub.y_axis);
        if (!std::isfinite(ty)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double yn = (ty - ymin) / (ymax - ymin);
        if (sub.y_axis.inverted) {
          yn = 1.0 - yn;
        }
        return bottom - yn * plot_h;
      };

      const Trace2D* categorical_trace = nullptr;
      for (const auto& t : sub.traces) {
        if (!t.x_labels.empty() && t.x_labels.size() == t.x.size() && t.x_labels.size() <= 12) {
          categorical_trace = &t;
          break;
        }
      }

      const auto y_ticks = resolve_ticks_2d(sub.y_axis, ymin, ymax);
      for (const auto& tick : y_ticks) {
        double yn = (tick.position - ymin) / (ymax - ymin);
        if (sub.y_axis.inverted) {
          yn = 1.0 - yn;
        }
        const double py = bottom - yn * plot_h;
        surface.stroke_line(left, py, right, py,
                            StrokeStyle2D{.color = "#e7ebf3", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
        surface.stroke_line(left - 5.0, py, left, py,
                            StrokeStyle2D{.color = "#718096", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
        surface.text(left - 9.0, py + 3.5, tick.label,
                     TextStyle2D{
                         .color = "#5b677a",
                         .size = 10.0,
                         .alpha = 1.0,
                         .bold = false,
                         .anchor = TextAnchor2D::Right,
                         .orientation = TextOrientation2D::Horizontal,
                     });
      }

      if (categorical_trace) {
        const auto x_ticks = resolve_ticks_2d(sub.x_axis, xmin, xmax, &categorical_trace->x, &categorical_trace->x_labels);
        for (const auto& tick : x_ticks) {
          const double px = sx(tick.position);
          if (!std::isfinite(px)) {
            continue;
          }
          surface.stroke_line(px, top, px, bottom,
                              StrokeStyle2D{.color = "#f0f2f7", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
          surface.stroke_line(px, bottom, px, bottom + 5.0,
                              StrokeStyle2D{.color = "#718096", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
          surface.text(px, bottom + 18.0, tick.label,
                       TextStyle2D{
                           .color = "#5b677a",
                           .size = 10.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Center,
                           .orientation = TextOrientation2D::Horizontal,
                       });
        }
      } else {
        const auto x_ticks = resolve_ticks_2d(sub.x_axis, xmin, xmax);
        for (const auto& tick : x_ticks) {
          double xn = (tick.position - xmin) / (xmax - xmin);
          if (sub.x_axis.inverted) {
            xn = 1.0 - xn;
          }
          const double px = left + xn * plot_w;
          surface.stroke_line(px, top, px, bottom,
                              StrokeStyle2D{.color = "#f0f2f7", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
          surface.stroke_line(px, bottom, px, bottom + 5.0,
                              StrokeStyle2D{.color = "#718096", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"});
          surface.text(px, bottom + 18.0, tick.label,
                       TextStyle2D{
                           .color = "#5b677a",
                           .size = 10.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Center,
                           .orientation = TextOrientation2D::Horizontal,
                       });
        }
      }

      surface.stroke_line(left, bottom, right, bottom,
                          StrokeStyle2D{.color = "#2d3748", .line_width = 1.2, .alpha = 1.0, .line_style = "solid"});
      surface.stroke_line(left, top, left, bottom,
                          StrokeStyle2D{.color = "#2d3748", .line_width = 1.2, .alpha = 1.0, .line_style = "solid"});

      const PlotTransform2D transform{
          .xmin = xmin,
          .xmax = xmax,
          .ymin = ymin,
          .ymax = ymax,
          .left = left,
          .right = right,
          .top = top,
          .bottom = bottom,
          .x_axis = &sub.x_axis,
          .y_axis = &sub.y_axis,
      };

      for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
        const auto& t = sub.traces[ti];
        const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
        Trace2D styled_trace = t;
        if (styled_trace.style.color.empty()) {
          styled_trace.style.color = color;
        }
        artist_for_trace_2d(styled_trace.kind).draw(surface, styled_trace, transform);
      }

      if (sub.legend.visible && !sub.traces.empty()) {
        std::size_t longest = sub.legend.title.size();
        for (const auto& trace : sub.traces) {
          longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
        }
        const bool has_title = !sub.legend.title.empty();
        const double legend_width = legend_width_hint_2d(sub.traces.size(), longest, has_title);
        const double legend_height = legend_height_hint_2d(sub.traces.size(), has_title);
        const auto [lx, ly] = map_legend_anchor_2d(
            sub.legend.location,
            static_cast<double>(left),
            static_cast<double>(right),
            static_cast<double>(top),
            static_cast<double>(bottom),
            legend_width,
            legend_height);

        surface.fill_rect(lx, ly, legend_width, legend_height, FillStyle2D{.color = "white", .alpha = 0.92}, 8.0);
        surface.stroke_rect(lx, ly, legend_width, legend_height,
                            StrokeStyle2D{.color = "#cfd6e3", .line_width = 1.0, .alpha = 1.0, .line_style = "solid"},
                            8.0);

        double y = ly + 14.0;
        const LegendArtist2D legend_artist{};
        if (has_title) {
          surface.text(lx + 10.0, y, sub.legend.title,
                       TextStyle2D{
                           .color = "#1f2937",
                           .size = 11.0,
                           .alpha = 1.0,
                           .bold = true,
                           .anchor = TextAnchor2D::Left,
                           .orientation = TextOrientation2D::Horizontal,
                       });
          y += 14.0;
        }
        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          const auto& t = sub.traces[ti];
          const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
          const double sample_y = y + static_cast<double>(ti) * 16.0;
          legend_artist.draw_sample(surface, t, lx + 8.0, lx + 26.0, sample_y, color);
          surface.text(lx + 32.0, sample_y + 4.0, t.name.empty() ? std::string("trace") : t.name,
                       TextStyle2D{
                           .color = "#334155",
                           .size = 11.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Left,
                           .orientation = TextOrientation2D::Horizontal,
                       });
        }
      }
    }
  }
}

} // namespace detail
} // namespace pluss::plotism
