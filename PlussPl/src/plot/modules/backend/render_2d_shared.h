#pragma once

#include <limits>
#include <stdexcept>

#include "plot/modules/artists/artist_2d.h"
#include "plot/modules/backend.h"
#include "plot/modules/backend/layout_2d.h"
#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

inline void render_2d_scene(RenderSurface2D& surface, const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save: width/height too small");
  }

  const ResolvedTheme figure_theme =
      fig.subplots.empty() ? detail::resolve_theme(Style{}) : detail::resolve_theme(fig.subplots.front().style);
  surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                    FillStyle2D{.color = figure_theme.page_background, .alpha = 1.0});

  const auto layout = make_figure_layout_2d(fig, target);

  if (!fig.title.empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, layout.figure_title_y, fig.title,
                 TextStyle2D{
                     .color = figure_theme.title_color,
                     .size = 22.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = figure_theme.font_family,
                 });
  }

  for (int r = 0; r < fig.rows; ++r) {
    for (int c = 0; c < fig.cols; ++c) {
      const int idx = r * fig.cols + c;
      const auto& sub = fig.subplots[static_cast<std::size_t>(idx)];
      const ResolvedTheme subplot_theme = detail::resolve_theme(sub.style);
      const auto& subplot_layout = layout.subplots[static_cast<std::size_t>(idx)];
      const auto& cell = subplot_layout.cell;
      const auto& plot = subplot_layout.plot;
      const double left = plot.x;
      const double right = plot.x + plot.width;
      const double top = plot.y;
      const double bottom = plot.y + plot.height;
      const double plot_w = plot.width;
      const double plot_h = plot.height;

      surface.fill_rect(cell.x, cell.y, cell.width, cell.height,
                        FillStyle2D{.color = subplot_theme.panel_background, .alpha = 1.0},
                        12.0);
      surface.stroke_rect(cell.x, cell.y, cell.width, cell.height,
                          StrokeStyle2D{
                              .color = subplot_theme.panel_border,
                              .line_width = 1.0,
                              .alpha = 1.0,
                              .line_style = "solid",
                          },
                          12.0);

      surface.fill_rect(plot.x, plot.y, plot.width, plot.height,
                        FillStyle2D{.color = subplot_theme.plot_background, .alpha = 1.0});
      surface.stroke_rect(plot.x, plot.y, plot.width, plot.height,
                          StrokeStyle2D{
                              .color = subplot_theme.plot_border,
                              .line_width = 1.0,
                              .alpha = 1.0,
                              .line_style = "solid",
                          });

      if (!sub.title.empty()) {
        surface.text(cell.x + cell.width * 0.5, subplot_layout.title_y, sub.title,
                     TextStyle2D{
                         .color = subplot_theme.subplot_title_color,
                         .size = 14.0,
                         .alpha = 1.0,
                         .bold = true,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::Horizontal,
                         .font_family = subplot_theme.font_family,
                     });
      }
      if (!sub.x_axis.label.empty()) {
        surface.text(cell.x + cell.width * 0.5, subplot_layout.x_label_y, sub.x_axis.label,
                     TextStyle2D{
                         .color = subplot_theme.axis_label_color,
                         .size = 12.0,
                         .alpha = 1.0,
                         .bold = false,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::Horizontal,
                         .font_family = subplot_theme.font_family,
                     });
      }
      if (!sub.y_axis.label.empty()) {
        surface.text(subplot_layout.y_label_x, cell.y + cell.height * 0.5, sub.y_axis.label,
                     TextStyle2D{
                         .color = subplot_theme.axis_label_color,
                         .size = 12.0,
                         .alpha = 1.0,
                         .bold = false,
                         .anchor = TextAnchor2D::Center,
                         .orientation = TextOrientation2D::VerticalCcw,
                         .font_family = subplot_theme.font_family,
                     });
      }

      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();
      bool has_bar_like = false;
      for (const auto& t : sub.traces) {
        if (t.kind == Trace2DKind::Heatmap && t.cols > 0 && t.rows > 0) {
          for (std::size_t i = 0; i < t.x.size(); ++i) {
            const auto [lo, hi] = cell_bounds_2d(t.x, i);
            const double tlo = axis_transform_2d(lo, sub.x_axis);
            const double thi = axis_transform_2d(hi, sub.x_axis);
            if (std::isfinite(tlo)) xmin = std::min(xmin, tlo);
            if (std::isfinite(thi)) xmax = std::max(xmax, thi);
          }
          for (std::size_t i = 0; i < t.y2.size(); ++i) {
            const auto [lo, hi] = cell_bounds_2d(t.y2, i);
            const double tlo = axis_transform_2d(lo, sub.y_axis);
            const double thi = axis_transform_2d(hi, sub.y_axis);
            if (std::isfinite(tlo)) ymin = std::min(ymin, tlo);
            if (std::isfinite(thi)) ymax = std::max(ymax, thi);
          }
          continue;
        }

        const std::size_t n = std::min(t.x.size(), t.y.size());
        if (!t.x_labels.empty() && t.x_labels.size() == t.x.size()) {
          for (std::size_t i = 0; i < t.x.size(); ++i) {
            const auto [lo, hi] = (t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) ? bar_bounds_2d(t, i)
                                                                                               : cell_bounds_2d(t.x, i);
            const double tlo = axis_transform_2d(lo, sub.x_axis);
            const double thi = axis_transform_2d(hi, sub.x_axis);
            if (std::isfinite(tlo)) xmin = std::min(xmin, tlo);
            if (std::isfinite(thi)) xmax = std::max(xmax, thi);
          }
        }
        if (t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) {
          has_bar_like = true;
          for (std::size_t i = 0; i < n; ++i) {
            const auto [lo, hi] = bar_bounds_2d(t, i);
            const double tlo = axis_transform_2d(lo, sub.x_axis);
            const double thi = axis_transform_2d(hi, sub.x_axis);
            if (std::isfinite(tlo)) xmin = std::min(xmin, tlo);
            if (std::isfinite(thi)) xmax = std::max(xmax, thi);
          }
        }
        for (std::size_t i = 0; i < n; ++i) {
          const double tx = axis_transform_2d(t.x[i], sub.x_axis);
          const double ty = axis_transform_2d(t.y[i], sub.y_axis);
          if (std::isfinite(tx) && t.kind != Trace2DKind::Bar && t.kind != Trace2DKind::Hist) {
            xmin = std::min(xmin, tx);
            xmax = std::max(xmax, tx);
          }
          if (std::isfinite(ty)) {
            ymin = std::min(ymin, ty);
            ymax = std::max(ymax, ty);
          }
        }
        if (t.kind != Trace2DKind::Bar) {
          for (double extra_x : t.x2) {
            const double tx = axis_transform_2d(extra_x, sub.x_axis);
            if (std::isfinite(tx)) {
              xmin = std::min(xmin, tx);
              xmax = std::max(xmax, tx);
            }
          }
        }
        if (t.kind == Trace2DKind::ErrorBar) {
          const std::size_t m = std::min(t.y.size(), t.y2.size());
          for (std::size_t i = 0; i < m; ++i) {
            const double lo = axis_transform_2d(t.y[i] - t.y2[i], sub.y_axis);
            const double hi = axis_transform_2d(t.y[i] + t.y2[i], sub.y_axis);
            if (std::isfinite(lo)) {
              ymin = std::min(ymin, lo);
              ymax = std::max(ymax, lo);
            }
            if (std::isfinite(hi)) {
              ymin = std::min(ymin, hi);
              ymax = std::max(ymax, hi);
            }
          }
        } else {
          for (double extra_y : t.y2) {
            const double ty = axis_transform_2d(extra_y, sub.y_axis);
            if (std::isfinite(ty)) {
              ymin = std::min(ymin, ty);
              ymax = std::max(ymax, ty);
            }
          }
        }
        if ((t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) && sub.y_axis.scale == AxisScale::Linear) {
          ymin = std::min(ymin, 0.0);
        }
      }

      if (has_bar_like && sub.y_axis.scale == AxisScale::Linear && !sub.y_axis.max.has_value() && std::isfinite(ymax)) {
        const double span = std::max(0.5, ymax - ymin);
        ymax += span * 0.08;
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

      std::vector<double> categorical_x_positions{};
      std::vector<std::string> categorical_x_labels{};
      std::size_t categorical_x_count = 0;
      const Trace2D* categorical_y_trace = nullptr;
      std::size_t longest_x_label = 0;
      std::size_t longest_y_label = 0;
      for (const auto& t : sub.traces) {
        if (!t.x_labels.empty() && t.x_labels.size() == t.x.size() && t.x_labels.size() <= 12) {
          if (categorical_x_labels.empty()) {
            categorical_x_labels = t.x_labels;
            categorical_x_positions = t.x;
            categorical_x_count = 1;
          } else if (t.x_labels == categorical_x_labels && t.x.size() == categorical_x_positions.size()) {
            for (std::size_t i = 0; i < t.x.size(); ++i) {
              categorical_x_positions[i] += t.x[i];
            }
            ++categorical_x_count;
          }
          for (const auto& label : t.x_labels) {
            longest_x_label = std::max(longest_x_label, label.size());
          }
        }
        if (!t.y_labels.empty() && t.y_labels.size() == t.y2.size() && t.y_labels.size() <= 12) {
          categorical_y_trace = &t;
          for (const auto& label : t.y_labels) {
            longest_y_label = std::max(longest_y_label, label.size());
          }
        }
        if (!categorical_x_labels.empty() && categorical_y_trace) {
          break;
        }
      }
      if (categorical_x_count > 1) {
        for (auto& position : categorical_x_positions) {
          position /= static_cast<double>(categorical_x_count);
        }
      }
      const double x_tick_size = longest_x_label > 6 ? 9.0 : 10.0;
      const double y_tick_size = longest_y_label > 6 ? 9.0 : 10.0;

      if (categorical_y_trace) {
        const auto y_ticks = resolve_ticks_2d(sub.y_axis, ymin, ymax, &categorical_y_trace->y2, &categorical_y_trace->y_labels);
        for (const auto& tick : y_ticks) {
          const double py = sy(tick.position);
          if (!std::isfinite(py)) {
            continue;
          }
          surface.stroke_line(left, py, right, py,
                              StrokeStyle2D{
                                  .color = subplot_theme.grid_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.stroke_line(left - 5.0, py, left, py,
                              StrokeStyle2D{
                                  .color = subplot_theme.tick_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.text(left - 9.0, py + 3.5, tick.label,
                       TextStyle2D{
                           .color = subplot_theme.tick_label_color,
                           .size = y_tick_size,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Right,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
                       });
        }
      } else {
        const auto y_ticks = resolve_ticks_2d(sub.y_axis, ymin, ymax);
        for (const auto& tick : y_ticks) {
          double yn = (tick.position - ymin) / (ymax - ymin);
          if (sub.y_axis.inverted) {
            yn = 1.0 - yn;
          }
          const double py = bottom - yn * plot_h;
          surface.stroke_line(left, py, right, py,
                              StrokeStyle2D{
                                  .color = subplot_theme.grid_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.stroke_line(left - 5.0, py, left, py,
                              StrokeStyle2D{
                                  .color = subplot_theme.tick_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.text(left - 9.0, py + 3.5, tick.label,
                       TextStyle2D{
                           .color = subplot_theme.tick_label_color,
                           .size = 10.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Right,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
                       });
        }
      }

      if (!categorical_x_labels.empty()) {
        const auto x_ticks = resolve_ticks_2d(sub.x_axis, xmin, xmax, &categorical_x_positions, &categorical_x_labels);
        for (const auto& tick : x_ticks) {
          const double px = sx(tick.position);
          if (!std::isfinite(px)) {
            continue;
          }
          surface.stroke_line(px, top, px, bottom,
                              StrokeStyle2D{
                                  .color = subplot_theme.grid_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.stroke_line(px, bottom, px, bottom + 5.0,
                              StrokeStyle2D{
                                  .color = subplot_theme.tick_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.text(px, bottom + 18.0, tick.label,
                       TextStyle2D{
                           .color = subplot_theme.tick_label_color,
                           .size = x_tick_size,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Center,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
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
                              StrokeStyle2D{
                                  .color = subplot_theme.grid_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.stroke_line(px, bottom, px, bottom + 5.0,
                              StrokeStyle2D{
                                  .color = subplot_theme.tick_color,
                                  .line_width = 1.0,
                                  .alpha = 1.0,
                                  .line_style = "solid",
                              });
          surface.text(px, bottom + 18.0, tick.label,
                       TextStyle2D{
                           .color = subplot_theme.tick_label_color,
                           .size = 10.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Center,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
                       });
        }
      }

      surface.stroke_line(left, bottom, right, bottom,
                          StrokeStyle2D{
                              .color = subplot_theme.axis_color,
                              .line_width = 1.2,
                              .alpha = 1.0,
                              .line_style = "solid",
                          });
      surface.stroke_line(left, top, left, bottom,
                          StrokeStyle2D{
                              .color = subplot_theme.axis_color,
                              .line_width = 1.2,
                              .alpha = 1.0,
                              .line_style = "solid",
                          });

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
        Trace2D styled_trace = sub.traces[ti];
        detail::apply_trace_style_defaults(styled_trace, subplot_theme, ti);
        artist_for_trace_2d(styled_trace.kind).draw(surface, styled_trace, transform);
      }

      if (subplot_layout.colorbar_box.has_value() && sub.colorbar.has_value() && sub.colorbar->visible) {
        const Trace2D* heatmap_trace = nullptr;
        std::size_t heatmap_index = 0;
        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          if (sub.traces[ti].kind == Trace2DKind::Heatmap && !sub.traces[ti].values.empty()) {
            heatmap_trace = &sub.traces[ti];
            heatmap_index = ti;
            break;
          }
        }
        if (heatmap_trace != nullptr) {
          Trace2D styled_trace = *heatmap_trace;
          detail::apply_trace_style_defaults(styled_trace, subplot_theme, heatmap_index);
          Colorbar styled_colorbar = *sub.colorbar;
          if (styled_colorbar.title.empty()) {
            styled_colorbar.title = styled_trace.name;
          }
          const auto& box = *subplot_layout.colorbar_box;
          const ColorbarArtist2D colorbar_artist{};
          colorbar_artist.draw(surface, box.x, box.y, box.width, box.height, styled_trace, styled_colorbar, subplot_theme);
        }
      }

      if (sub.legend.visible && !sub.traces.empty()) {
        std::size_t longest = sub.legend.title.size();
        for (const auto& trace : sub.traces) {
          longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
        }
        const bool has_title = !sub.legend.title.empty();
        const double legend_width = subplot_layout.legend_box.has_value()
                                        ? subplot_layout.legend_box->width
                                        : legend_width_hint_2d(sub.traces.size(), longest, has_title);
        const double legend_height = subplot_layout.legend_box.has_value()
                                         ? subplot_layout.legend_box->height
                                         : legend_height_hint_2d(sub.traces.size(), has_title);
        const auto [lx, ly] = subplot_layout.legend_box.has_value()
                                  ? std::pair<double, double>{subplot_layout.legend_box->x, subplot_layout.legend_box->y}
                                  : map_legend_anchor_2d(
                                        sub.legend.location,
                                        static_cast<double>(left),
                                        static_cast<double>(right),
                                        static_cast<double>(top),
                                        static_cast<double>(bottom),
                                        legend_width,
                                        legend_height);

        surface.fill_rect(lx, ly, legend_width, legend_height,
                          FillStyle2D{.color = subplot_theme.legend_background, .alpha = 0.92},
                          8.0);
        surface.stroke_rect(lx, ly, legend_width, legend_height,
                            StrokeStyle2D{
                                .color = subplot_theme.legend_border,
                                .line_width = 1.0,
                                .alpha = 1.0,
                                .line_style = "solid",
                            },
                            8.0);

        double y = ly + 14.0;
        const LegendArtist2D legend_artist{};
        if (has_title) {
          surface.text(lx + 10.0, y, sub.legend.title,
                       TextStyle2D{
                           .color = subplot_theme.legend_title_color,
                           .size = 11.0,
                           .alpha = 1.0,
                           .bold = true,
                           .anchor = TextAnchor2D::Left,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
                       });
          y += 14.0;
        }
        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          Trace2D styled_trace = sub.traces[ti];
          detail::apply_trace_style_defaults(styled_trace, subplot_theme, ti);
          const double sample_y = y + static_cast<double>(ti) * 16.0;
          legend_artist.draw_sample(surface, styled_trace, lx + 8.0, lx + 26.0, sample_y, styled_trace.style.color);
          surface.text(lx + 32.0, sample_y + 4.0, styled_trace.name.empty() ? std::string("trace") : styled_trace.name,
                       TextStyle2D{
                           .color = subplot_theme.legend_text_color,
                           .size = 11.0,
                           .alpha = 1.0,
                           .bold = false,
                           .anchor = TextAnchor2D::Left,
                           .orientation = TextOrientation2D::Horizontal,
                           .font_family = subplot_theme.font_family,
                       });
        }
      }
    }
  }
}

} // namespace detail
} // namespace pluss::plotism
