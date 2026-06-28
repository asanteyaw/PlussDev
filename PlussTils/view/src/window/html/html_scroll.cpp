#include "pluss/tils/view/window/html_scroll.h"

#include <algorithm>
#include <cmath>

namespace pluss::view::window {

namespace {

[[nodiscard]] bool contains(const RectI rect, const PointI point) {
  return !rect.empty() && point.x >= rect.x && point.y >= rect.y &&
         point.x < rect.x + rect.width && point.y < rect.y + rect.height;
}

[[nodiscard]] double clamp_offset(const double value, const int limit) {
  if (!std::isfinite(value) || limit <= 0) {
    return 0.0;
  }
  return std::clamp(value, 0.0, static_cast<double>(limit));
}

[[nodiscard]] int rounded_offset(const double value, const int limit) {
  return static_cast<int>(std::lround(clamp_offset(value, limit)));
}

[[nodiscard]] double apply_delta(double& value, const int limit, const double delta) {
  if (!std::isfinite(delta) || delta == 0.0 || limit <= 0) {
    value = clamp_offset(value, limit);
    return delta;
  }
  const double before = clamp_offset(value, limit);
  const double after = clamp_offset(before + delta, limit);
  value = after;
  return delta - (after - before);
}

} // namespace

void HtmlScrollModel::update_metrics(const HtmlRenderStats& metrics) {
  metrics_ = metrics;
  document_.x = clamp_offset(document_.x, metrics_.document_max_scroll_x);
  document_.y = clamp_offset(document_.y, metrics_.document_max_scroll_y);

  for (const auto& region : metrics_.table_scroll_regions) {
    auto& offset = tables_[region.target];
    offset.x = clamp_offset(offset.x, region.max_scroll_x);
    offset.y = clamp_offset(offset.y, region.max_scroll_y);
  }
  if (scrollbar_drag_.surface == DragSurface::Document) {
    scrollbar_drag_.track = scrollbar_drag_.axis == DragAxis::Horizontal
                                ? metrics_.document_horizontal_track_rect
                                : metrics_.document_vertical_track_rect;
    scrollbar_drag_.thumb = scrollbar_drag_.axis == DragAxis::Horizontal
                                ? metrics_.document_horizontal_thumb_rect
                                : metrics_.document_vertical_thumb_rect;
  } else if (scrollbar_drag_.surface == DragSurface::Table) {
    const auto region_it = std::find_if(
        metrics_.table_scroll_regions.begin(),
        metrics_.table_scroll_regions.end(),
        [&](const HtmlScrollRegion& region) { return region.target == scrollbar_drag_.target; });
    if (region_it == metrics_.table_scroll_regions.end()) {
      scrollbar_drag_ = ScrollbarDrag{};
    } else {
      scrollbar_drag_.track = scrollbar_drag_.axis == DragAxis::Horizontal
                                  ? region_it->horizontal_track_rect
                                  : region_it->vertical_track_rect;
      scrollbar_drag_.thumb = scrollbar_drag_.axis == DragAxis::Horizontal
                                  ? region_it->horizontal_thumb_rect
                                  : region_it->vertical_thumb_rect;
    }
  }
}

void HtmlScrollModel::reset() noexcept {
  document_ = Offset{};
  for (auto& [target, offset] : tables_) {
    (void)target;
    offset = Offset{};
  }
  scrollbar_drag_ = ScrollbarDrag{};
  pointer_is_down_ = false;
  suppress_content_drag_ = false;
}

bool HtmlScrollModel::scroll_wheel(double delta_x,
                                   double delta_y,
                                   const PointI location,
                                   const bool precise_delta,
                                   const bool shift_modifier) {
  if (!std::isfinite(delta_x) || !std::isfinite(delta_y)) {
    return false;
  }
  last_pointer_ = location;

  if (shift_modifier && std::abs(delta_x) < std::abs(delta_y)) {
    delta_x = delta_y;
    delta_y = 0.0;
  }

  const double dominant_delta = std::max(std::abs(delta_x), std::abs(delta_y));
  const double scale =
      precise_delta ? std::clamp(1.35 + dominant_delta / 30.0, 1.35, 2.0) : 72.0;
  delta_x *= -scale;
  delta_y *= -scale;

  if (!precise_delta) {
    if (std::abs(delta_x) > std::abs(delta_y)) {
      delta_y = 0.0;
    } else {
      delta_x = 0.0;
    }
  }

  bool changed = false;
  const auto apply_axis = [&](const bool horizontal, const double delta) {
    if (delta == 0.0) {
      return;
    }

    double remaining = delta;
    for (auto it = metrics_.table_scroll_regions.rbegin();
         it != metrics_.table_scroll_regions.rend();
         ++it) {
      if (!contains(it->viewport_rect, location)) {
        continue;
      }
      const int limit = horizontal ? it->max_scroll_x : it->max_scroll_y;
      if (limit > 0) {
        auto& offset = tables_[it->target];
        double& value = horizontal ? offset.x : offset.y;
        const double before = value;
        remaining = apply_delta(value, limit, remaining);
        changed = changed || value != before;
      }
      break;
    }

    const int document_limit =
        horizontal ? metrics_.document_max_scroll_x : metrics_.document_max_scroll_y;
    if (document_limit > 0 && std::abs(remaining) > 1e-9) {
      double& value = horizontal ? document_.x : document_.y;
      const double before = value;
      (void)apply_delta(value, document_limit, remaining);
      changed = changed || value != before;
    }
  };

  apply_axis(true, delta_x);
  apply_axis(false, delta_y);
  return changed;
}

bool HtmlScrollModel::pointer_down(const PointI location) {
  last_pointer_ = location;
  pointer_is_down_ = true;
  scrollbar_drag_ = ScrollbarDrag{};
  suppress_content_drag_ = false;

  const auto activate_or_page = [&](const DragSurface surface,
                                    const std::string& target,
                                    const DragAxis axis,
                                    const RectI track,
                                    const RectI thumb,
                                    double& offset,
                                    const int limit,
                                    const int viewport_span) {
    if (track.empty() || limit <= 0 || !contains(track, location)) {
      return false;
    }
    if (contains(thumb, location)) {
      scrollbar_drag_ = ScrollbarDrag{surface, axis, target, track, thumb};
      return true;
    }

    const int coordinate = axis == DragAxis::Horizontal ? location.x : location.y;
    const int thumb_start = axis == DragAxis::Horizontal ? thumb.x : thumb.y;
    const int thumb_end = thumb_start + (axis == DragAxis::Horizontal ? thumb.width : thumb.height);
    const double page = std::max(1.0, static_cast<double>(viewport_span) * 0.9);
    const double delta = coordinate < thumb_start ? -page : (coordinate >= thumb_end ? page : 0.0);
    (void)apply_delta(offset, limit, delta);
    suppress_content_drag_ = true;
    return true;
  };

  for (auto it = metrics_.table_scroll_regions.rbegin();
       it != metrics_.table_scroll_regions.rend();
       ++it) {
    if (!contains(it->viewport_rect, location)) {
      continue;
    }
    auto& offset = tables_[it->target];
    if (activate_or_page(DragSurface::Table,
                         it->target,
                         DragAxis::Horizontal,
                         it->horizontal_track_rect,
                         it->horizontal_thumb_rect,
                         offset.x,
                         it->max_scroll_x,
                         it->viewport_rect.width)) {
      return true;
    }
    if (activate_or_page(DragSurface::Table,
                         it->target,
                         DragAxis::Vertical,
                         it->vertical_track_rect,
                         it->vertical_thumb_rect,
                         offset.y,
                         it->max_scroll_y,
                         it->viewport_rect.height)) {
      return true;
    }
    break;
  }

  if (activate_or_page(DragSurface::Document,
                       {},
                       DragAxis::Horizontal,
                       metrics_.document_horizontal_track_rect,
                       metrics_.document_horizontal_thumb_rect,
                       document_.x,
                       metrics_.document_max_scroll_x,
                       metrics_.document_viewport_rect.width)) {
    return true;
  }
  return activate_or_page(DragSurface::Document,
                          {},
                          DragAxis::Vertical,
                          metrics_.document_vertical_track_rect,
                          metrics_.document_vertical_thumb_rect,
                          document_.y,
                          metrics_.document_max_scroll_y,
                          metrics_.document_viewport_rect.height);
}

bool HtmlScrollModel::pointer_drag(const double delta_x,
                                   const double delta_y,
                                   const PointI location) {
  if (!pointer_is_down_) {
    last_pointer_ = location;
    return false;
  }

  double movement_x = static_cast<double>(location.x - last_pointer_.x);
  double movement_y = static_cast<double>(location.y - last_pointer_.y);
  if (last_pointer_.x < 0 || last_pointer_.y < 0) {
    movement_x = delta_x;
    movement_y = delta_y;
  }
  last_pointer_ = location;

  if (scrollbar_drag_.surface != DragSurface::None) {
    const bool horizontal = scrollbar_drag_.axis == DragAxis::Horizontal;
    const int track_span = horizontal ? scrollbar_drag_.track.width : scrollbar_drag_.track.height;
    const int thumb_span = horizontal ? scrollbar_drag_.thumb.width : scrollbar_drag_.thumb.height;
    const int travel = std::max(1, track_span - thumb_span);
    const double movement = horizontal ? movement_x : movement_y;

    if (scrollbar_drag_.surface == DragSurface::Document) {
      const int limit =
          horizontal ? metrics_.document_max_scroll_x : metrics_.document_max_scroll_y;
      double& value = horizontal ? document_.x : document_.y;
      const double before = value;
      (void)apply_delta(value, limit, movement * static_cast<double>(limit) / travel);
      return value != before;
    }

    const auto region_it = std::find_if(
        metrics_.table_scroll_regions.begin(),
        metrics_.table_scroll_regions.end(),
        [&](const HtmlScrollRegion& region) { return region.target == scrollbar_drag_.target; });
    if (region_it == metrics_.table_scroll_regions.end()) {
      scrollbar_drag_ = ScrollbarDrag{};
      return false;
    }
    auto& offset = tables_[region_it->target];
    const int limit = horizontal ? region_it->max_scroll_x : region_it->max_scroll_y;
    double& value = horizontal ? offset.x : offset.y;
    const double before = value;
    (void)apply_delta(value, limit, movement * static_cast<double>(limit) / travel);
    return value != before;
  }

  if (suppress_content_drag_) {
    return false;
  }
  return scroll_wheel(movement_x, movement_y, location, true, false);
}

void HtmlScrollModel::pointer_up() noexcept {
  pointer_is_down_ = false;
  scrollbar_drag_ = ScrollbarDrag{};
  suppress_content_drag_ = false;
}

bool HtmlScrollModel::command(const std::string_view command_name) {
  if (command_name == "reset_view") {
    const auto before = snapshot();
    reset();
    return before.document.x != 0 || before.document.y != 0 ||
           std::any_of(before.tables.begin(), before.tables.end(), [](const auto& item) {
             return item.second.x != 0 || item.second.y != 0;
           });
  }

  PointI location = last_pointer_;
  if (location.x < 0 || location.y < 0) {
    const auto viewport = metrics_.document_viewport_rect;
    location = PointI{viewport.x + viewport.width / 2, viewport.y + viewport.height / 2};
  }

  const bool horizontal = command_name == "pan_left" || command_name == "pan_right";
  const bool toward_end =
      command_name == "pan_right" || command_name == "pan_down" ||
      command_name == "page_down" || command_name == "next_page" ||
      command_name == "end";
  const bool page =
      command_name == "page_up" || command_name == "page_down" ||
      command_name == "next_page" || command_name == "previous_page";
  const bool absolute = command_name == "home" || command_name == "end";

  if (!horizontal && command_name != "pan_up" && command_name != "pan_down" &&
      !page && !absolute) {
    return false;
  }

  double* value = horizontal ? &document_.x : &document_.y;
  int limit = horizontal ? metrics_.document_max_scroll_x : metrics_.document_max_scroll_y;
  int viewport_span =
      horizontal ? metrics_.document_viewport_rect.width : metrics_.document_viewport_rect.height;
  bool targets_table = false;

  for (auto it = metrics_.table_scroll_regions.rbegin();
       it != metrics_.table_scroll_regions.rend();
       ++it) {
    if (!contains(it->viewport_rect, location)) {
      continue;
    }
    const int table_limit = horizontal ? it->max_scroll_x : it->max_scroll_y;
    if (table_limit > 0) {
      auto& offset = tables_[it->target];
      value = horizontal ? &offset.x : &offset.y;
      limit = table_limit;
      viewport_span = horizontal ? it->viewport_rect.width : it->viewport_rect.height;
      targets_table = true;
    }
    break;
  }

  if (limit <= 0) {
    return false;
  }
  const double before = *value;
  const double document_before = horizontal ? document_.x : document_.y;
  if (absolute) {
    *value = toward_end ? static_cast<double>(limit) : 0.0;
  } else {
    const double step =
        page ? std::max(1.0, static_cast<double>(viewport_span) * 0.9)
             : std::clamp(static_cast<double>(viewport_span) * 0.1, 32.0, 80.0);
    const double remaining = apply_delta(*value, limit, toward_end ? step : -step);
    if (targets_table && std::abs(remaining) > 1e-9) {
      double& document_value = horizontal ? document_.x : document_.y;
      const int document_limit =
          horizontal ? metrics_.document_max_scroll_x : metrics_.document_max_scroll_y;
      (void)apply_delta(document_value, document_limit, remaining);
    }
  }
  const bool target_changed = *value != before;
  if (!targets_table) {
    return target_changed;
  }
  return target_changed || (horizontal ? document_.x : document_.y) != document_before;
}

PointI HtmlScrollModel::document_offset_px() const noexcept {
  return PointI{
      rounded_offset(document_.x, metrics_.document_max_scroll_x),
      rounded_offset(document_.y, metrics_.document_max_scroll_y),
  };
}

std::unordered_map<std::string, PointI> HtmlScrollModel::table_offsets_px() const {
  std::unordered_map<std::string, PointI> out;
  out.reserve(tables_.size());
  for (const auto& [target, offset] : tables_) {
    const auto region_it = std::find_if(
        metrics_.table_scroll_regions.begin(),
        metrics_.table_scroll_regions.end(),
        [&](const HtmlScrollRegion& region) { return region.target == target; });
    if (region_it == metrics_.table_scroll_regions.end()) {
      out.emplace(
          target,
          PointI{
              static_cast<int>(std::lround(std::max(0.0, offset.x))),
              static_cast<int>(std::lround(std::max(0.0, offset.y))),
          });
      continue;
    }
    out.emplace(target,
                PointI{
                    rounded_offset(offset.x, region_it->max_scroll_x),
                    rounded_offset(offset.y, region_it->max_scroll_y),
                });
  }
  return out;
}

HtmlScrollSnapshot HtmlScrollModel::snapshot() const {
  return HtmlScrollSnapshot{
      .document = document_offset_px(),
      .tables = table_offsets_px(),
  };
}

} // namespace pluss::view::window
