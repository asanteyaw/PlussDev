#include "table_chrome.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace pluss::view::window::detail {

namespace {

void fill(Scene& scene, const RectI rect, const Color color, const int radius) {
  if (rect.empty()) {
    return;
  }
  if (radius > 0) {
    scene.add(FillRoundedRectCommand{rect, radius, color});
  } else {
    scene.add(FillRectCommand{rect, color});
  }
}

// A thumb's length along a track: in proportion to how much of the content the window shows, never shorter
// than the geometry's minimum and never longer than the track.
int thumb_length(const int track, const int view, const std::int64_t content, const int minimum) {
  const double ratio = static_cast<double>(view) / static_cast<double>(std::max<std::int64_t>(1, content));
  return std::min(track, std::max(minimum, static_cast<int>(std::lround(static_cast<double>(track) * ratio))));
}

// Where a thumb of `thumb` sits on a track of `track` when `scroll` of `max_scroll` is scrolled.
int thumb_offset(const int track, const int thumb, const int scroll, const int max_scroll) {
  return static_cast<int>(std::lround(static_cast<double>(std::max(0, track - thumb)) * static_cast<double>(scroll) /
                                      static_cast<double>(std::max(1, max_scroll))));
}

}  // namespace

void draw_table_scrollbars(Scene& scene,
                           const HtmlRenderConfig& config,
                           const HtmlTableGeometryPx& geo,
                           const TablePaint& paint,
                           const std::string& table_target,
                           const RectI& interactive_viewport,
                           const TableScrollbars& bars) {
  const int thickness = geo.scrollbar;
  const int radius = thickness / 2;
  const int inset = geo.scrollbar_inset;
  RectI horizontal_track{};
  RectI horizontal_thumb{};
  RectI vertical_track{};
  RectI vertical_thumb{};
  if (bars.max_scroll_x > 0) {
    // Anchored to the content's real bottom edge, so the track can never sit on the last row; the outer box's
    // edge only moves it further out when there is room.
    const int below_content = bars.viewport.y + bars.viewport.height + inset;
    const int near_outer_edge = bars.outer.y + bars.outer.height - std::max(geo.scrollbar_edge, bars.padding / 2 + thickness);
    const int track_y = std::max(below_content, near_outer_edge);
    const int track_w = std::max(geo.scrollbar_min_track, bars.view_w - 2 * inset);
    const int track_x = bars.viewport.x + bars.frozen_w + inset;
    horizontal_track = RectI{track_x, track_y, track_w, thickness};
    const int thumb_w = thumb_length(track_w, bars.view_w, bars.content_w, geo.scrollbar_min_thumb);
    horizontal_thumb = RectI{track_x + thumb_offset(track_w, thumb_w, bars.scroll_x, bars.max_scroll_x), track_y, thumb_w, thickness};
    if (config.draw_scrollbars) {
      fill(scene, horizontal_track, paint.track, radius);
      fill(scene, horizontal_thumb, paint.thumb, radius);
    }
  }
  if (bars.max_scroll_y > 0) {
    const int right_of_content = bars.viewport.x + bars.viewport.width + inset;
    const int near_outer_edge = bars.outer.x + bars.outer.width - std::max(geo.scrollbar_edge, bars.padding / 2 + thickness);
    const int track_x = std::max(right_of_content, near_outer_edge);
    const int track_h = std::max(geo.scrollbar_min_track, bars.body_view_h - 2 * inset);
    const int track_y = bars.viewport.y + bars.header_h + inset;
    vertical_track = RectI{track_x, track_y, thickness, track_h};
    const int thumb_h = thumb_length(track_h, bars.body_view_h, bars.body_h, geo.scrollbar_min_thumb);
    vertical_thumb = RectI{track_x, track_y + thumb_offset(track_h, thumb_h, bars.scroll_y, bars.max_scroll_y), thickness, thumb_h};
    if (config.draw_scrollbars) {
      fill(scene, vertical_track, paint.track, radius);
      fill(scene, vertical_thumb, paint.thumb, radius);
    }
  }
  if (config.render_stats != nullptr) {
    config.render_stats->table_scroll_regions.push_back(HtmlScrollRegion{
        .target = table_target,
        .viewport_rect = interactive_viewport,
        .max_scroll_x = bars.max_scroll_x,
        .max_scroll_y = bars.max_scroll_y,
        .horizontal_track_rect = horizontal_track,
        .horizontal_thumb_rect = horizontal_thumb,
        .vertical_track_rect = vertical_track,
        .vertical_thumb_rect = vertical_thumb,
    });
  }
}

HeaderControls::HeaderControls(Scene& scene,
                               const HtmlRenderConfig& config,
                               std::string table_target,
                               const HtmlTableGeometryPx& geometry)
    : scene_(scene),
      config_(config),
      target_(std::move(table_target)),
      geo_(geometry),
      sort_target_matches_(config.table_sort_target.empty() || config.table_sort_target == target_) {}

void HeaderControls::add(const Cell& cell, const int row_y, const int row_h) {
  const int x0 = std::max(cell.x, cell.clip_left);
  const int x1 = std::min(cell.x + cell.width, cell.clip_right);
  if (config_.hit_regions != nullptr && x1 > x0) {
    config_.hit_regions->push_back(HtmlHitRegion{
        .rect = RectI{x0, row_y, x1 - x0, row_h},
        .action = "table.sort.column",
        .target = target_,
        .value = static_cast<int>(cell.column),
    });
  }

  // Sort arrows: a stacked pair of triangles at the right of the header; the active direction of the sorted
  // column is drawn in full strength, every other arrow faint.
  if (config_.table_sort_controls && cell.sortable) {
    const bool active = sort_target_matches_ && config_.table_sort_column >= 0 &&
                        cell.column == static_cast<std::size_t>(config_.table_sort_column);
    // Inactive arrows: the text colour mixed most of the way into the header's own background, so they recede
    // in any theme.
    const auto mix = [](const int a, const int b) { return static_cast<std::uint8_t>((a * 35 + b * 65) / 100); };
    const Color faint = Color::rgba(mix(cell.ink.r, cell.paper.r), mix(cell.ink.g, cell.paper.g),
                                    mix(cell.ink.b, cell.paper.b), 255);
    const int control_x = cell.x + cell.width - geo_.sort_pad - geo_.sort_control;
    const int centre_x = control_x + geo_.sort_control / 2;
    const int mid_y = row_y + row_h / 2;
    for (int k = 0; k < 2; ++k) {
      const bool up = k == 0;
      const bool lit = active && (config_.table_sort_ascending == up);
      const int base_y = up ? mid_y - geo_.sort_triangle_gap / 2 : mid_y + (geo_.sort_triangle_gap - geo_.sort_triangle_gap / 2);
      const int tip_y = up ? base_y - geo_.sort_triangle_height : base_y + geo_.sort_triangle_height;
      scene_.add(FillPolygonCommand{
          {PointI{centre_x - geo_.sort_triangle_base / 2, base_y},
           PointI{centre_x + geo_.sort_triangle_base / 2, base_y},
           PointI{centre_x, tip_y}},
          lit ? cell.ink : faint});
      if (config_.hit_regions != nullptr) {
        const int cx0 = std::max(control_x, cell.clip_left);
        const int cx1 = std::min(control_x + geo_.sort_control, cell.clip_right);
        if (cx1 > cx0) {
          const int y0 = up ? row_y : mid_y;
          const int y1 = up ? mid_y : row_y + row_h;
          config_.hit_regions->push_back(HtmlHitRegion{
              .rect = RectI{cx0, y0, cx1 - cx0, y1 - y0},
              .action = up ? "table.sort.asc" : "table.sort.desc",
              .target = target_,
              .value = static_cast<int>(cell.column),
          });
        }
      }
    }
  }

  // The drag handle on the right edge, with the highlight it shows while the pointer is over it.
  if (cell.resizable) {
    const int edge = cell.x + cell.width;
    const int hx0 = std::max(edge - kResizeHandlePx / 2, cell.clip_left);
    const int hx1 = std::min(edge + kResizeHandlePx / 2, cell.clip_right);
    if (hx1 > hx0) {
      handles_.push_back(HtmlHitRegion{.rect = RectI{hx0, row_y, hx1 - hx0, row_h},
                                       .action = "table.resize.column",
                                       .target = target_,
                                       .value = static_cast<int>(cell.column),
                                       .aux = cell.width});
      looks_.push_back(Look{edge, cell.column, cell.ink});
    }
  }
}

void HeaderControls::finish(const int row_y, const int row_h) {
  if (!config_.hover_resize_target.empty() && config_.hover_resize_target == target_) {
    for (const auto& look : looks_) {
      if (static_cast<int>(look.column) == config_.hover_resize_column) {
        scene_.add(LineCommand{PointI{look.edge, row_y}, PointI{look.edge, row_y + row_h}, 3, look.ink});
      }
    }
  }
  if (config_.hit_regions != nullptr) {
    for (auto& handle : handles_) {
      config_.hit_regions->push_back(std::move(handle));
    }
  }
  handles_.clear();
  looks_.clear();
}

}  // namespace pluss::view::window::detail
