// Views about which series are shown and how they are arranged: isolate, focus, window, layout.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

constexpr std::size_t kMaxSelectable = 24;

[[nodiscard]] bool selectable(const Trace& trace) {
  if (!trace.visible() || Access::derived(trace)) {
    return false;
  }
  switch (trace.kind()) {
    case TraceKind::Trend:
    case TraceKind::Cloud:
    case TraceKind::Columns:
    case TraceKind::Ribbon:
    case TraceKind::Spike:
    case TraceKind::Bucket:
      return true;
    default:
      return false;
  }
}

// The pane the series views work on: the first one that holds several series (a facet grid's single-series panes are
// left alone). Returns -1 when there is none.
[[nodiscard]] int crowded_pane(const Studio& studio) {
  int index = 0;
  for (const Pane& pane : studio.panes()) {
    std::size_t count = 0;
    for (const Trace& trace : pane.traces()) {
      count += selectable(trace) ? 1U : 0U;
    }
    if (count >= 2 && count <= kMaxSelectable && pane.kind() == PaneKind::Planar) {
      return index;
    }
    ++index;
  }
  return -1;
}

[[nodiscard]] std::vector<Option> series_options(const Studio& studio, std::string_view first_label,
                                                std::string_view prefix) {
  std::vector<Option> out;
  const int at = crowded_pane(studio);
  if (at < 0) {
    return out;
  }
  out.push_back({"all", std::string(first_label)});
  std::size_t local = 0;
  for (const Trace& trace : studio.panes()[static_cast<std::size_t>(at)].traces()) {
    if (selectable(trace)) {
      out.push_back({"t" + std::to_string(local), std::string(prefix) + series_name(trace, local)});
      ++local;
    }
  }
  return out;
}

[[nodiscard]] std::size_t chosen_index(std::string_view id) {
  return id.size() > 1 && id.front() == 't' ? static_cast<std::size_t>(std::stoul(std::string(id.substr(1)))) : 0;
}

// ---- window ---------------------------------------------------------------------------------------------------------

void crop_pane(Pane& pane, double fraction, bool keep_last) {
  double low = std::numeric_limits<double>::infinity();
  double high = -low;
  for (Trace* trace : series_of(pane)) {
    const auto ch = channels(*trace);
    if (ch.horizontal) {
      continue;
    }
    for (const double v : to_vec(*ch.pos)) {
      if (std::isfinite(v)) {
        low = std::min(low, v);
        high = std::max(high, v);
      }
    }
  }
  if (!(high > low)) {
    return;
  }
  const double span = high - low;
  const double from = keep_last ? high - fraction * span : low;
  const double to = keep_last ? high : low + fraction * span;
  const double slack = 1e-9 * span;
  for (Trace* trace : series_of(pane)) {
    const auto ch = channels(*trace);
    if (ch.horizontal) {
      continue;
    }
    const Vec pos = to_vec(*ch.pos);
    std::vector<bool> keep(pos.size());
    for (std::size_t i = 0; i < pos.size(); ++i) {
      keep[i] = std::isfinite(pos[i]) && pos[i] >= from - slack && pos[i] <= to + slack;
    }
    filter_points(*trace, keep);
  }
  auto& axis = Access::x_axis(pane);
  if (axis.ticks.has_value()) {
    const Vec positions = to_vec(axis.ticks->positions);
    Vec kept;
    std::vector<std::string> labels;
    for (std::size_t i = 0; i < positions.size() && i < axis.ticks->labels.size(); ++i) {
      if (positions[i] >= from - slack && positions[i] <= to + slack) {
        kept.push_back(positions[i]);
        labels.push_back(axis.ticks->labels[i]);
      }
    }
    axis.ticks = AxisTicks{to_series(kept), std::move(labels)};
  }
}

// ---- layout ---------------------------------------------------------------------------------------------------------

[[nodiscard]] bool splittable(const Studio& studio) {
  if (studio.pane_count() != 1 || studio.panes().front().kind() != PaneKind::Planar) {
    return false;
  }
  std::size_t count = 0;
  for (const Trace& trace : studio.panes().front().traces()) {
    if (Access::derived(trace)) {
      continue;
    }
    if (!is_series(trace)) {
      return false;  // a histogram, box or other kind next to the lines: leave the pane whole
    }
    ++count;
  }
  return count >= 2 && count <= 12;
}

void extend_range(const Series& series, double& low, double& high) {
  for (const double v : to_vec(series)) {
    if (std::isfinite(v)) {
      low = std::min(low, v);
      high = std::max(high, v);
    }
  }
}

void apply_layout(Studio& studio, std::string_view id, const Choices&) {
  auto& panes = Access::panes(studio);
  const Pane source = panes.front();

  // Each series with the traces drawn from it (markers, fits) that follow it.
  std::vector<std::vector<Trace>> parts;
  std::vector<Trace> waiting;  // traces drawn behind a series come first in the deque but belong to the next one
  for (const Trace& trace : source.traces()) {
    if (Access::behind(trace)) {
      waiting.push_back(trace);
      continue;
    }
    if (!Access::derived(trace) || parts.empty()) {
      parts.emplace_back(std::move(waiting));
      waiting.clear();
    }
    parts.back().push_back(trace);
  }
  if (parts.size() < 2) {
    return;
  }
  const auto count = static_cast<std::int64_t>(parts.size());
  const bool stack = id != "grid";
  const std::int64_t columns = stack ? 1 : (count <= 4 ? 2 : 3);
  const std::int64_t rows = (count + columns - 1) / columns;
  const bool share_y = id == "stack_y";

  double x_low = std::numeric_limits<double>::infinity();
  double x_high = -x_low;
  double y_low = x_low;
  double y_high = -x_low;
  bool same_x = true;
  Vec first_x;
  for (const auto& part : parts) {
    Trace probe = *std::find_if(part.begin(), part.end(), [](const Trace& t) { return !Access::derived(t); });
    const auto ch = channels(probe);
    if (!ch) {
      continue;
    }
    extend_range(*ch.pos, x_low, x_high);
    extend_range(*ch.val, y_low, y_high);
    if (probe.kind() == TraceKind::Columns || probe.kind() == TraceKind::Ribbon || probe.kind() == TraceKind::Spike) {
      y_low = std::min(y_low, 0.0);
      y_high = std::max(y_high, 0.0);
    }
    const Vec x = to_vec(*ch.pos);
    if (first_x.empty()) {
      first_x = x;
    } else if (x != first_x) {
      same_x = false;
    }
  }

  panes.clear();
  std::int64_t at = 0;
  for (auto& part : parts) {
    Pane pane = source;
    auto& traces = Access::traces(pane);
    traces.clear();
    for (auto& trace : part) {
      traces.push_back(std::move(trace));
    }
    pane.title(series_name(*std::find_if(traces.begin(), traces.end(), [](const Trace& t) { return !Access::derived(t); }),
                           static_cast<std::size_t>(at)));
    pane.legend(KeyPlace::None);
    const std::int64_t row = at / columns;
    const std::int64_t col = at % columns;
    const bool bottom_row = row == rows - 1 || at + columns >= count;
    if (Access::y_axis(pane).label.size() > 8) {
      // A long label does not fit beside a short panel: it moves into the panel's title.
      pane.title(pane.title_text() + " - " + Access::y_axis(pane).label);
      pane.y_label("");
    }
    if (stack && !bottom_row) {
      pane.x_tick_labels(false);
      pane.x_label("");
    }
    if (!same_x && x_high > x_low && !Access::x_axis(pane).ticks.has_value()) {
      pane.x_limits({x_low, x_high});
    }
    if (share_y && y_high > y_low) {
      pane.y_limits({y_low, y_high});
    }
    pane.place(row, col);
    panes.push_back(std::move(pane));
    ++at;
  }
  studio.mosaic(rows, columns);
  if (stack) {
    studio.size(studio.extent().width,
                std::clamp(std::max(studio.extent().height, 210.0 * static_cast<double>(count) + 140.0), 300.0, 1700.0));
  }
}

} // namespace

// Keeps the points of `trace` flagged in `keep`.
void filter_points(Trace& trace, const std::vector<bool>& keep) {
  const auto pick = [&](const Series& series) {
    const Vec all = to_vec(series);
    Vec out;
    for (std::size_t i = 0; i < all.size() && i < keep.size(); ++i) {
      if (keep[i]) {
        out.push_back(all[i]);
      }
    }
    return to_series(out);
  };
  const auto pick_optional = [&](std::optional<Series>& series) {
    if (series.has_value() && static_cast<std::size_t>(series->numel()) == keep.size()) {
      series = pick(*series);
    }
  };
  std::visit(
      [&](auto& d) {
        using D = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<D, TrendData> || std::is_same_v<D, RibbonData> || std::is_same_v<D, SpikeData> ||
                      std::is_same_v<D, ColumnsData>) {
          d.x = pick(d.x);
          d.y = pick(d.y);
        } else if constexpr (std::is_same_v<D, CloudData>) {
          d.x = pick(d.x);
          d.y = pick(d.y);
          pick_optional(d.size);
          pick_optional(d.color);
        }
      },
      Access::payload(trace));
}

void register_series_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      100, "isolate", "Isolate", "Series",
      [](const Studio& studio) { return series_options(studio, "All series", "Only "); },
      [](Studio& studio, std::string_view id, const Choices&) {
        const int at = crowded_pane(studio);
        if (at < 0) {
          return;
        }
        const std::size_t keep = chosen_index(id);
        auto& traces = Access::traces(Access::panes(studio)[static_cast<std::size_t>(at)]);
        std::size_t local = 0;
        for (auto it = traces.begin(); it != traces.end();) {
          if (selectable(*it) && local++ != keep) {
            it = traces.erase(it);
          } else {
            ++it;
          }
        }
      }});

  defs.push_back(Def{
      110, "focus", "Focus", "Series",
      [](const Studio& studio) { return series_options(studio, "No focus", ""); },
      [](Studio& studio, std::string_view id, const Choices&) {
        const int at = crowded_pane(studio);
        if (at < 0) {
          return;
        }
        const std::size_t target = chosen_index(id);
        std::size_t local = 0;
        for (Trace& trace : Access::traces(Access::panes(studio)[static_cast<std::size_t>(at)])) {
          if (!selectable(trace)) {
            continue;
          }
          const bool chosen = local++ == target;
          Rgba color = drawn_color(trace);
          if (chosen) {
            trace.width(std::max(trace.stroke_width() * 1.6F, 2.6F));
            trace.stroke(color);
          } else {
            trace.stroke(with_alpha(color, color.alpha * 0.25));
            trace.width(std::max(trace.stroke_width() * 0.8F, 0.8F));
          }
        }
      }});

  defs.push_back(Def{
      300, "window", "Window", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        for (const Pane& pane : studio.panes()) {
          for (const Trace* trace : series_of(pane)) {
            if (!is_horizontal(*trace) && point_count(*trace) >= 12) {
              out = {{"all", "Everything"},   {"last50", "Last 50%"},   {"last25", "Last 25%"},
                     {"last10", "Last 10%"}, {"first50", "First 50%"}, {"first25", "First 25%"}};
              return out;
            }
          }
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        const double fraction = id.ends_with("50") ? 0.5 : id.ends_with("25") ? 0.25 : 0.1;
        for (Pane& pane : Access::panes(studio)) {
          crop_pane(pane, fraction, id.starts_with("last"));
        }
      }});

  defs.push_back(Def{
      400, "layout", "Layout", "Series",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (splittable(studio)) {
          out = {{"overlay", "Overlaid"}, {"stack", "One panel per series"},
                 {"stack_y", "Panels, same y range"}, {"grid", "Grid of panels"}};
        }
        return out;
      },
      apply_layout});
}

} // namespace pluss::pl::views::internal
