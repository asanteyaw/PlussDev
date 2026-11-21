// Views that change the values a figure shows: sort, orient, transform, smooth, draw.

#include "views_internal.h"

namespace pluss::pl::views::internal {

namespace {

// ---- sort -----------------------------------------------------------------------------------------------------------

[[nodiscard]] AxisState& category_axis(Pane& pane, bool horizontal) {
  return horizontal ? Access::y_axis(pane) : Access::x_axis(pane);
}

[[nodiscard]] std::vector<Trace*> bars_of(Pane& pane) {
  std::vector<Trace*> out;
  for (Trace* trace : series_of(pane)) {
    if (trace->kind() == TraceKind::Columns) {
      out.push_back(trace);
    }
  }
  return out;
}

[[nodiscard]] bool sortable(Pane& pane) {
  const auto bars = bars_of(pane);
  if (bars.empty()) {
    return false;
  }
  const bool horizontal = is_horizontal(*bars.front());
  const auto& axis = category_axis(pane, horizontal);
  const auto n = static_cast<std::size_t>(point_count(*bars.front()));
  return axis.ticks.has_value() && axis.ticks->labels.size() == n && n >= 2 && n <= 400;
}

void sort_bars(Pane& pane, std::string_view id) {
  auto bars = bars_of(pane);
  const bool horizontal = is_horizontal(*bars.front());
  auto& axis = category_axis(pane, horizontal);
  const auto lead = channels(*bars.front());
  const Vec positions = to_vec(*lead.pos);
  const Vec values = to_vec(*lead.val);
  const std::vector<std::string> tick_labels = axis.ticks->labels;
  const Vec tick_positions = to_vec(axis.ticks->positions);
  const std::size_t n = positions.size();

  // The label of each bar: the tick at its position.
  std::vector<std::string> names(n);
  for (std::size_t i = 0; i < n; ++i) {
    std::size_t best = std::min(i, tick_labels.size() - 1);
    for (std::size_t k = 0; k < tick_positions.size() && k < tick_labels.size(); ++k) {
      if (std::abs(tick_positions[k] - positions[i]) < 1e-9) {
        best = k;
        break;
      }
    }
    names[i] = tick_labels[best];
  }

  std::vector<std::size_t> order(n);
  std::iota(order.begin(), order.end(), std::size_t{0});
  const auto value_key = [&](std::size_t i) { return std::isfinite(values[i]) ? values[i] : -std::numeric_limits<double>::infinity(); };
  if (id == "asc") {
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return value_key(a) < value_key(b); });
  } else if (id == "desc") {
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return value_key(a) > value_key(b); });
  } else if (id == "az") {
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return names[a] < names[b]; });
  } else {
    std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return names[a] > names[b]; });
  }
  // The reading order of a horizontal chart runs from the top, which is its highest position.
  Vec slots = positions;
  std::sort(slots.begin(), slots.end());
  if (horizontal) {
    std::reverse(order.begin(), order.end());
  }

  std::vector<std::string> sorted_names(n);
  for (std::size_t j = 0; j < n; ++j) {
    sorted_names[j] = names[order[j]];
  }
  for (Trace* bar : bars) {
    const auto ch = channels(*bar);
    if (static_cast<std::size_t>(ch.val->numel()) != n) {
      continue;
    }
    const Vec v = to_vec(*ch.val);
    Vec reordered(n);
    for (std::size_t j = 0; j < n; ++j) {
      reordered[j] = v[order[j]];
    }
    *ch.pos = to_series(slots);
    *ch.val = to_series(reordered);
  }
  axis.ticks = AxisTicks{to_series(slots), std::move(sorted_names)};
}

// ---- orient ---------------------------------------------------------------------------------------------------------

[[nodiscard]] bool all_bars(Pane& pane) {
  const auto series = series_of(pane);
  return !series.empty() &&
         std::all_of(series.begin(), series.end(), [](const Trace* t) { return t->kind() == TraceKind::Columns; });
}

void flip_pane(Pane& pane) {
  for (Trace& trace : Access::traces(pane)) {
    if (auto* d = std::get_if<ColumnsData>(&Access::payload(trace))) {
      std::swap(d->x, d->y);
      d->horizontal = !d->horizontal;
    }
  }
  std::swap(Access::x_axis(pane), Access::y_axis(pane));
  for (auto& rule : Access::rules(pane)) {
    rule.axis = rule.axis == CueAxis::X ? CueAxis::Y : rule.axis == CueAxis::Y ? CueAxis::X : rule.axis;
  }
  for (auto& band : Access::bands(pane)) {
    band.axis = band.axis == CueAxis::X ? CueAxis::Y : band.axis == CueAxis::Y ? CueAxis::X : band.axis;
  }
  for (auto& pin : Access::pins(pane)) {
    std::swap(pin.anchor.x, pin.anchor.y);
  }
}

// ---- transform ------------------------------------------------------------------------------------------------------

struct Transformed {
  Vec values;
  bool drop_first{false};
};

[[nodiscard]] std::optional<Transformed> transformed(const Vec& y, std::string_view id) {
  Transformed out;
  out.values = y;
  const std::size_t n = y.size();
  if (id == "index" || id == "pct") {
    double first = kNaN;
    for (const double v : y) {
      if (std::isfinite(v) && v != 0.0) {
        first = v;
        break;
      }
    }
    if (!std::isfinite(first)) {
      return std::nullopt;
    }
    for (double& v : out.values) {
      v = id == "index" ? v / first * 100.0 : (v / first - 1.0) * 100.0;
    }
  } else if (id == "step") {
    for (std::size_t i = n; i-- > 1;) {
      out.values[i] = y[i - 1] != 0.0 ? (y[i] / y[i - 1] - 1.0) * 100.0 : kNaN;
    }
    if (n > 0) {
      out.values[0] = kNaN;
    }
    out.drop_first = true;
  } else if (id == "diff") {
    for (std::size_t i = n; i-- > 1;) {
      out.values[i] = y[i] - y[i - 1];
    }
    if (n > 0) {
      out.values[0] = kNaN;
    }
    out.drop_first = true;
  } else if (id == "cumsum") {
    double running = 0.0;
    for (double& v : out.values) {
      if (std::isfinite(v)) {
        running += v;
      }
      v = running;
    }
  } else if (id == "zscore") {
    const Summary s = summarize(y);
    if (s.count < 2 || !(s.sd > 0.0)) {
      return std::nullopt;
    }
    for (double& v : out.values) {
      v = (v - s.mean) / s.sd;
    }
  } else if (id == "minmax") {
    const Summary s = summarize(y);
    if (s.count < 2 || !(s.max > s.min)) {
      return std::nullopt;
    }
    for (double& v : out.values) {
      v = (v - s.min) / (s.max - s.min);
    }
  } else if (id == "share") {
    double total = 0.0;
    for (const double v : y) {
      total += std::isfinite(v) ? v : 0.0;
    }
    if (!(std::abs(total) > 0.0)) {
      return std::nullopt;
    }
    for (double& v : out.values) {
      v = v / total * 100.0;
    }
  } else {
    return std::nullopt;
  }
  return out;
}

[[nodiscard]] const char* transform_note(std::string_view id) {
  if (id == "index") return "index";
  if (id == "pct") return "% since start";
  if (id == "step") return "% change";
  if (id == "diff") return "change";
  if (id == "cumsum") return "running total";
  if (id == "zscore") return "z-score";
  if (id == "minmax") return "0 to 1";
  return "% of total";
}

void apply_transform(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    bool horizontal = false;
    bool changed = false;
    for (Trace& trace : Access::traces(pane)) {
      if (!is_series(trace) || trace.kind() == TraceKind::Cloud) {
        continue;
      }
      const auto ch = channels(trace);
      const auto result = transformed(to_vec(*ch.val), id);
      if (!result.has_value()) {
        continue;
      }
      Vec pos = to_vec(*ch.pos);
      Vec val = result->values;
      if (result->drop_first && !val.empty()) {
        val.erase(val.begin());
        pos.erase(pos.begin());
      }
      if (trace.kind() == TraceKind::Columns) {
        for (double& v : val) {
          v = std::isfinite(v) ? v : 0.0;
        }
      }
      *ch.pos = to_series(pos);
      *ch.val = to_series(val);
      if (ch.base != nullptr) {
        *ch.base = 0.0;
      }
      horizontal = ch.horizontal;
      changed = true;
    }
    if (changed) {
      auto& axis = horizontal ? Access::x_axis(pane) : Access::y_axis(pane);
      const std::string base = axis.label.empty() ? std::string("value") : axis.label;
      axis.label = base + " (" + transform_note(id) + ")";
      if (!horizontal) {
        pane.y_label_centered(true);  // a longer label reads better turned along the axis
      }
    }
  }
}

// ---- smooth ---------------------------------------------------------------------------------------------------------

[[nodiscard]] Vec moving_average(const Vec& y, std::size_t window) {
  const std::size_t n = y.size();
  const std::size_t half = window / 2;
  // Prefix sums over the finite values, so a window costs two lookups however wide it is.
  Vec sum(n + 1, 0.0);
  std::vector<std::size_t> count(n + 1, 0);
  for (std::size_t i = 0; i < n; ++i) {
    const bool ok = std::isfinite(y[i]);
    sum[i + 1] = sum[i] + (ok ? y[i] : 0.0);
    count[i + 1] = count[i] + (ok ? 1U : 0U);
  }
  Vec out(n, kNaN);
  for (std::size_t i = 0; i < n; ++i) {
    if (!std::isfinite(y[i])) {
      continue;
    }
    const std::size_t lo = i >= half ? i - half : 0;
    const std::size_t hi = std::min(n, i + half + 1);
    out[i] = (sum[hi] - sum[lo]) / static_cast<double>(count[hi] - count[lo]);
  }
  return out;
}

[[nodiscard]] Vec exponential_average(const Vec& y, double alpha) {
  Vec out(y.size(), kNaN);
  double level = kNaN;
  for (std::size_t i = 0; i < y.size(); ++i) {
    if (!std::isfinite(y[i])) {
      continue;
    }
    level = std::isfinite(level) ? alpha * y[i] + (1.0 - alpha) * level : y[i];
    out[i] = level;
  }
  return out;
}

[[nodiscard]] bool smoothable(const Trace& trace) {
  return is_series(trace) && (trace.kind() == TraceKind::Trend || trace.kind() == TraceKind::Ribbon) &&
         point_count(trace) >= 6;
}

[[nodiscard]] std::int64_t longest_series(const Studio& studio) {
  std::int64_t longest = 0;
  for (const Pane& pane : studio.panes()) {
    for (const Trace& trace : pane.traces()) {
      if (smoothable(trace)) {
        longest = std::max(longest, point_count(trace));
      }
    }
  }
  return longest;
}

void apply_smooth(Studio& studio, std::string_view id, const Choices& all) {
  const auto ghost = all.find("ghost");
  const bool keep_raw = ghost != all.end() && ghost->second == "faint";
  for (Pane& pane : Access::panes(studio)) {
    auto& traces = Access::traces(pane);
    for (std::size_t i = 0; i < traces.size(); ++i) {
      if (!smoothable(traces[i])) {
        continue;
      }
      Trace& trace = traces[i];
      const auto ch = channels(trace);
      const Vec raw = to_vec(*ch.val);
      Vec smooth;
      if (id.starts_with("ma")) {
        smooth = moving_average(raw, static_cast<std::size_t>(std::stoul(std::string(id.substr(2)))));
      } else {
        smooth = exponential_average(raw, id == "ema_fast" ? 0.4 : 0.12);
      }
      const Series raw_x = *ch.pos;
      const Series raw_y = *ch.val;
      const Rgba color = drawn_color(trace);
      const float width = trace.stroke_width();
      const bool filled = trace.kind() == TraceKind::Ribbon;
      *ch.val = to_series(smooth);
      if (keep_raw) {
        // The raw values stay as a faint line behind the smoothed series (and are skipped by every later view).
        Trace ghost_line = make_derived(TraceKind::Trend, TrendData{raw_x, raw_y}, {}, with_alpha(color, 0.28),
                                        std::max(0.9F, width * 0.7F), true);
        (void)filled;
        traces.insert(traces.begin() + static_cast<std::ptrdiff_t>(i), std::move(ghost_line));
        ++i;
      }
    }
  }
}

// ---- draw -----------------------------------------------------------------------------------------------------------

[[nodiscard]] bool drawable(const Trace& trace) {
  return is_series(trace) &&
         (trace.kind() == TraceKind::Trend || trace.kind() == TraceKind::Ribbon || trace.kind() == TraceKind::Spike) &&
         point_count(trace) >= 2;
}

void apply_draw(Studio& studio, std::string_view id, const Choices&) {
  for (Pane& pane : Access::panes(studio)) {
    auto& traces = Access::traces(pane);
    // Filled areas and stems hang from a baseline: zero when the values come near it, otherwise just under the data, so
    // that the picture is not a tall block of colour with the interesting part along its top.
    double low = std::numeric_limits<double>::infinity();
    double high = -low;
    std::size_t drawn = 0;
    for (Trace& trace : traces) {
      if (!drawable(trace)) {
        continue;
      }
      ++drawn;
      for (const double v : to_vec(*channels(trace).val)) {
        if (std::isfinite(v)) {
          low = std::min(low, v);
          high = std::max(high, v);
        }
      }
    }
    const bool near_zero = !(low > 0.0) || low <= 0.4 * high;
    const double floor_value = near_zero ? std::min(0.0, low) : low - 0.04 * (high - low);
    for (std::size_t i = 0; i < traces.size(); ++i) {
      if (!drawable(traces[i])) {
        continue;
      }
      Trace& old = traces[i];
      auto ch = channels(old);
      const Vec x = to_vec(*ch.pos);
      const Vec y = to_vec(*ch.val);
      const double base = ch.base != nullptr ? *ch.base : floor_value;
      const Rgba color = drawn_color(old);
      const float width = old.stroke_width();
      const std::string label = old.label_text();

      Vec xs = x;
      Vec ys = y;
      if (id == "step") {
        xs.clear();
        ys.clear();
        for (std::size_t k = 0; k < x.size(); ++k) {
          if (k > 0) {
            xs.push_back(x[k]);
            ys.push_back(y[k - 1]);
          }
          xs.push_back(x[k]);
          ys.push_back(y[k]);
        }
      }
      TraceKind kind = TraceKind::Trend;
      TracePayload payload = TrendData{to_series(xs), to_series(ys)};
      Rgba fill_color = color;
      if (id == "area") {
        if (drawn > 1) {
          fill_color = with_alpha(color, color.alpha * 0.5);  // overlapping areas must not hide each other
        }
        kind = TraceKind::Ribbon;
        Vec above(y.size());
        for (std::size_t k = 0; k < y.size(); ++k) {
          above[k] = y[k] - base;  // a ribbon's values are measured up from its base
        }
        payload = RibbonData{to_series(x), to_series(above), base};
      } else if (id == "stems") {
        kind = TraceKind::Spike;
        payload = SpikeData{to_series(x), to_series(y), base};
      } else if (id == "points") {
        kind = TraceKind::Cloud;
        payload = CloudData{to_series(x), to_series(y), std::nullopt, std::nullopt};
      }
      Trace made(kind, std::move(payload));
      made.label(label);
      made.stroke(fill_color);
      made.width(width);
      old = std::move(made);
      if (id == "linepoints") {
        Trace markers = make_derived(TraceKind::Cloud, CloudData{to_series(x), to_series(y), to_series(Vec{90.0}), std::nullopt},
                                     {}, color, width);
        traces.insert(traces.begin() + static_cast<std::ptrdiff_t>(i) + 1, std::move(markers));
        ++i;
      }
    }
  }
}

} // namespace

void register_data_views(std::vector<Def>& defs) {
  defs.push_back(Def{
      510, "sort", "Sort", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        Studio copy = studio;
        for (Pane& pane : Access::panes(copy)) {
          if (sortable(pane)) {
            out = {{"as_drawn", "As drawn"}, {"asc", "Smallest first"}, {"desc", "Largest first"},
                   {"az", "Label A to Z"},   {"za", "Label Z to A"}};
            break;
          }
        }
        return out;
      },
      [](Studio& studio, std::string_view id, const Choices&) {
        for (Pane& pane : Access::panes(studio)) {
          if (sortable(pane)) {
            sort_bars(pane, id);
          }
        }
      }});

  defs.push_back(Def{
      500, "orient", "Orientation", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        Studio copy = studio;
        for (Pane& pane : Access::panes(copy)) {
          if (all_bars(pane)) {
            const bool horizontal = is_horizontal(*series_of(pane).front());
            out = {{"as_drawn", "As drawn"}, {"flip", horizontal ? "Vertical bars" : "Horizontal bars"}};
            break;
          }
        }
        return out;
      },
      [](Studio& studio, std::string_view, const Choices&) {
        for (Pane& pane : Access::panes(studio)) {
          if (all_bars(pane)) {
            flip_pane(pane);
          }
        }
      }});

  defs.push_back(Def{
      600, "transform", "Transform", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        for (const Pane& pane : studio.panes()) {
          for (const Trace* trace : series_of(pane)) {
            if (trace->kind() != TraceKind::Cloud && point_count(*trace) >= 3) {
              return std::vector<Option>{{"raw", "As drawn"},       {"index", "Index (start = 100)"},
                                         {"pct", "% change since start"}, {"step", "% change per step"},
                                         {"diff", "Change per step"}, {"cumsum", "Running total"},
                                         {"zscore", "Z-score"},       {"minmax", "Scaled 0 to 1"},
                                         {"share", "Share of total (%)"}};
            }
          }
        }
        return out;
      },
      apply_transform});

  defs.push_back(Def{
      700, "smooth", "Smooth", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        const std::int64_t n = longest_series(studio);
        if (n < 6) {
          return out;
        }
        out.push_back({"raw", "As drawn"});
        for (const std::size_t w : {3U, 7U, 15U, 31U, 61U}) {
          if (static_cast<std::int64_t>(w) <= n / 2) {
            out.push_back({"ma" + std::to_string(w), "Moving average (" + std::to_string(w) + ")"});
          }
        }
        out.push_back({"ema_fast", "Exponential (fast)"});
        out.push_back({"ema_slow", "Exponential (slow)"});
        return out;
      },
      apply_smooth});

  defs.push_back(Def{
      705, "ghost", "Raw data", "Data",
      [](const Studio& studio) {
        std::vector<Option> out;
        if (longest_series(studio) >= 6) {
          out = {{"hidden", "Hidden"}, {"faint", "Faint, behind the smoothed line"}};
        }
        return out;
      },
      [](Studio&, std::string_view, const Choices&) {}});  // read by the Smooth group

  defs.push_back(Def{
      800, "draw", "Draw as", "Draw",
      [](const Studio& studio) {
        std::vector<Option> out;
        for (const Pane& pane : studio.panes()) {
          for (const Trace& trace : pane.traces()) {
            if (drawable(trace)) {
              return std::vector<Option>{{"as_drawn", "As drawn"}, {"line", "Line"},   {"step", "Steps"},
                                         {"area", "Filled area"},  {"stems", "Stems"}, {"points", "Points"},
                                         {"linepoints", "Line with points"}};
            }
          }
        }
        return out;
      },
      apply_draw});
}

} // namespace pluss::pl::views::internal
