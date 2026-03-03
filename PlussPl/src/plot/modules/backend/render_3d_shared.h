#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "plot/modules/artists/artist_2d.h"
#include "plot/modules/artists/artist_3d.h"
#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/projector_3d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

inline void render_3d_scene(RenderSurface2D& surface, const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save: width/height too small");
  }

  const ResolvedTheme theme = detail::resolve_theme(fig.style);
  surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                    FillStyle2D{.color = theme.page_background, .alpha = 1.0});
  if (!fig.title.empty()) {
    surface.text(static_cast<double>(target.width) * 0.5, 28.0, fig.title,
                 TextStyle2D{
                     .color = theme.title_color,
                     .size = 22.0,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }

  double legend_width = 0.0;
  double legend_height = 0.0;
  if (fig.legend.visible && !fig.traces.empty()) {
    std::size_t longest = fig.legend.title.size();
    for (const auto& trace : fig.traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !fig.legend.title.empty();
    legend_width = legend_width_hint_2d(fig.traces.size(), longest, has_title);
    legend_height = legend_height_hint_2d(fig.traces.size(), has_title);
  }
  const bool outside_legend = fig.legend.location == LegendLocation::OutsideRight && legend_width > 0.0;
  const Trace3D* surface_for_colorbar = nullptr;
  if (fig.colorbar.has_value() && fig.colorbar->visible) {
    auto it = std::find_if(fig.traces.begin(), fig.traces.end(), [](const Trace3D& trace) {
      return trace.kind == Trace3DKind::Surface && !trace.z.empty();
    });
    if (it != fig.traces.end()) {
      surface_for_colorbar = &(*it);
    }
  }
  const bool show_colorbar = surface_for_colorbar != nullptr;
  const double colorbar_width = show_colorbar ? 64.0 : 0.0;
  const double aux_gap = (show_colorbar && outside_legend) ? 12.0 : 0.0;

  const double left = 110.0;
  const double right = static_cast<double>(target.width - 90) - (outside_legend ? (legend_width + 16.0) : 0.0) -
                       (show_colorbar ? (colorbar_width + 16.0 + aux_gap) : 0.0);
  const double top = 84.0;
  const double bottom = static_cast<double>(target.height - 96);
  const double pw = std::max(1.0, right - left);
  const double ph = std::max(1.0, bottom - top);

  surface.fill_rect(left, top, pw, ph, FillStyle2D{.color = theme.panel_background, .alpha = 1.0}, 14.0);
  surface.stroke_rect(left, top, pw, ph,
                      StrokeStyle2D{
                          .color = theme.panel_border,
                          .line_width = 1.0,
                          .alpha = 1.0,
                          .line_style = "solid",
                      },
                      14.0);

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = axis_transform_2d(t.x[i], fig.x_axis);
      const double ty = axis_transform_2d(t.y[i], fig.y_axis);
      const double tz = axis_transform_2d(t.z[i], fig.z_axis);
      if (std::isfinite(tx)) {
        xmin = std::min(xmin, tx);
        xmax = std::max(xmax, tx);
      }
      if (std::isfinite(ty)) {
        ymin = std::min(ymin, ty);
        ymax = std::max(ymax, ty);
      }
      if (std::isfinite(tz)) {
        zmin = std::min(zmin, tz);
        zmax = std::max(zmax, tz);
      }
    }
  }

  if (fig.x_axis.min.has_value()) xmin = axis_transform_2d(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = axis_transform_2d(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = axis_transform_2d(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = axis_transform_2d(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = axis_transform_2d(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = axis_transform_2d(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = ensure_limits_2d(xmin, xmax);
  std::tie(ymin, ymax) = ensure_limits_2d(ymin, ymax);
  std::tie(zmin, zmax) = ensure_limits_2d(zmin, zmax);

  const PlotTransform3D transform{
      .xmin = xmin,
      .xmax = xmax,
      .ymin = ymin,
      .ymax = ymax,
      .zmin = zmin,
      .zmax = zmax,
      .left = left,
      .top = top,
      .width = pw,
      .height = ph,
      .x_axis = &fig.x_axis,
      .y_axis = &fig.y_axis,
      .z_axis = &fig.z_axis,
      .camera = &fig.camera,
  };

  const std::array<std::array<double, 3>, 8> corners{{
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.0, 1.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0},
      {1.0, 0.0, 1.0},
      {1.0, 1.0, 1.0},
      {0.0, 1.0, 1.0},
  }};
  const std::array<std::pair<int, int>, 12> edges{{
      {0, 1}, {1, 2}, {2, 3}, {3, 0},
      {4, 5}, {5, 6}, {6, 7}, {7, 4},
      {0, 4}, {1, 5}, {2, 6}, {3, 7},
  }};

  for (const auto& edge : edges) {
    const auto p1 = transform.project_normalized(corners[edge.first][0], corners[edge.first][1], corners[edge.first][2]);
    const auto p2 = transform.project_normalized(corners[edge.second][0], corners[edge.second][1], corners[edge.second][2]);
    const bool primary = edge == std::pair<int, int>{0, 1} || edge == std::pair<int, int>{0, 3} ||
                         edge == std::pair<int, int>{0, 4};
    surface.stroke_line(p1.x, p1.y, p2.x, p2.y,
                        StrokeStyle2D{
                            .color = primary ? theme.axis_color : theme.grid_color,
                            .line_width = primary ? 1.6 : 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });
  }

  const auto x_ticks = resolve_ticks_2d(fig.x_axis, xmin, xmax);
  const auto y_ticks = resolve_ticks_2d(fig.y_axis, ymin, ymax);
  const auto z_ticks = resolve_ticks_2d(fig.z_axis, zmin, zmax);

  for (const auto& tick : x_ticks) {
    const auto p = transform.project_normalized((tick.position - xmin) / (xmax - xmin), 0.0, 0.0);
    surface.stroke_line(p.x, p.y, p.x, p.y + 6.0,
                        StrokeStyle2D{
                            .color = theme.tick_color,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });
    surface.text(p.x, p.y + 20.0, tick.label,
                 TextStyle2D{
                     .color = theme.tick_label_color,
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }
  for (const auto& tick : y_ticks) {
    const auto p = transform.project_normalized(0.0, (tick.position - ymin) / (ymax - ymin), 0.0);
    surface.stroke_line(p.x - 6.0, p.y, p.x, p.y,
                        StrokeStyle2D{
                            .color = theme.tick_color,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });
    surface.text(p.x - 10.0, p.y + 3.5, tick.label,
                 TextStyle2D{
                     .color = theme.tick_label_color,
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }
  for (const auto& tick : z_ticks) {
    const auto p = transform.project_normalized(0.0, 0.0, (tick.position - zmin) / (zmax - zmin));
    surface.stroke_line(p.x - 5.0, p.y + 4.0, p.x, p.y,
                        StrokeStyle2D{
                            .color = theme.tick_color,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        });
    surface.text(p.x - 8.0, p.y - 6.0, tick.label,
                 TextStyle2D{
                     .color = theme.tick_label_color,
                     .size = 10.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }

  if (!fig.x_axis.label.empty()) {
    const auto p = transform.project_normalized(1.0, 0.0, 0.0);
    surface.text(p.x + 14.0, p.y + 22.0, fig.x_axis.label,
                 TextStyle2D{
                     .color = theme.axis_label_color,
                     .size = 12.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Left,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }
  if (!fig.y_axis.label.empty()) {
    const auto p = transform.project_normalized(0.0, 1.0, 0.0);
    surface.text(p.x - 18.0, p.y - 12.0, fig.y_axis.label,
                 TextStyle2D{
                     .color = theme.axis_label_color,
                     .size = 12.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }
  if (!fig.z_axis.label.empty()) {
    const auto p = transform.project_normalized(0.0, 0.0, 1.0);
    surface.text(p.x - 8.0, p.y - 14.0, fig.z_axis.label,
                 TextStyle2D{
                     .color = theme.axis_label_color,
                     .size = 12.0,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = TextAnchor2D::Right,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }

  DepthSorter3D depth_sorter{};
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    Trace3D styled_trace = fig.traces[ti];
    detail::apply_trace_style_defaults(styled_trace, theme, ti);
    artist_for_trace_3d(styled_trace.kind).emit(depth_sorter, styled_trace, transform, theme);
  }
  depth_sorter.paint(surface);

  if (show_colorbar) {
    Trace2D scalar_trace;
    scalar_trace.kind = Trace2DKind::Heatmap;
    scalar_trace.name = surface_for_colorbar->name;
    scalar_trace.values = surface_for_colorbar->z;
    scalar_trace.style = surface_for_colorbar->style;

    Colorbar colorbar = *fig.colorbar;
    if (colorbar.title.empty()) {
      colorbar.title = !surface_for_colorbar->name.empty() ? surface_for_colorbar->name : fig.z_axis.label;
    }

    const double colorbar_x = right + 14.0;
    const double colorbar_y = top + 6.0;
    const double colorbar_h = std::max(60.0, ph - 12.0);
    const ColorbarArtist2D colorbar_artist{};
    colorbar_artist.draw(surface, colorbar_x, colorbar_y, colorbar_width, colorbar_h, scalar_trace, colorbar, theme);
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    const bool has_title = !fig.legend.title.empty();
    const auto [lx, ly] = outside_legend
                              ? std::pair<double, double>{right + 14.0 + (show_colorbar ? (colorbar_width + 16.0 + aux_gap) : 0.0), top + 4.0}
                              : map_legend_anchor_2d(fig.legend.location, left, right, top, bottom, legend_width, legend_height);

    surface.fill_rect(lx, ly, legend_width, legend_height,
                      FillStyle2D{.color = theme.legend_background, .alpha = 0.92},
                      8.0);
    surface.stroke_rect(lx, ly, legend_width, legend_height,
                        StrokeStyle2D{
                            .color = theme.legend_border,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        },
                        8.0);
    double y = ly + 14.0;
    if (has_title) {
      surface.text(lx + 10.0, y, fig.legend.title,
                   TextStyle2D{
                       .color = theme.legend_title_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = true,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
      y += 14.0;
    }
    const LegendArtist3D legend_artist{};
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      Trace3D styled_trace = fig.traces[ti];
      detail::apply_trace_style_defaults(styled_trace, theme, ti);
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      legend_artist.draw_sample(surface, styled_trace, lx + 8.0, lx + 26.0, sample_y, theme);
      surface.text(lx + 32.0, sample_y + 4.0, styled_trace.name.empty() ? std::string("trace") : styled_trace.name,
                   TextStyle2D{
                       .color = theme.legend_text_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
    }
  }
}

} // namespace detail
} // namespace pluss::plotism
