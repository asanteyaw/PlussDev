#pragma once

// Shared machinery of the plot views (pluss/pl/views.h): access to the parts of a figure a view rewrites, the numeric
// helpers they share, and the definition of one view group. Each views_*.cpp registers its groups into one list.

#include "pluss/pl/flash/flash.h"
#include "pluss/pl/views.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::pl::detail {

// The one friend of Trace, Pane and Studio the view engine needs: views rewrite a copy of a figure's traces, axes and cues.
struct ViewAccess {
  static TracePayload& payload(Trace& trace) { return trace.payload_; }
  static bool derived(const Trace& trace) { return trace.derived_ != 0; }
  static bool behind(const Trace& trace) { return trace.derived_ == 2; }
  static void mark_derived(Trace& trace, bool behind = false) { trace.derived_ = behind ? 2 : 1; }
  static std::deque<Trace>& traces(Pane& pane) { return pane.traces_; }
  static std::deque<Pane>& panes(Studio& studio) { return studio.panes_; }
  static AxisState& x_axis(Pane& pane) { return pane.x_axis_; }
  static AxisState& y_axis(Pane& pane) { return pane.y_axis_; }
  static std::vector<RuleCue>& rules(Pane& pane) { return pane.rules_; }
  static std::vector<BandCue>& bands(Pane& pane) { return pane.bands_; }
  static std::vector<PinCue>& pins(Pane& pane) { return pane.pins_; }
};

} // namespace pluss::pl::detail

namespace pluss::pl::views::internal {

using Access = pluss::pl::detail::ViewAccess;
using Vec = std::vector<double>;

inline constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

[[nodiscard]] inline Vec to_vec(const Series& series) {
  if (series.numel() == 0) {
    return {};
  }
  return flash::detail::to_f64_vector(flash::detail::flatten_f64(series));
}

[[nodiscard]] inline Series to_series(const Vec& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
}

// The coordinate channels of a trace a view reads and writes: `pos` runs along the category / time axis and `val` along
// the value axis (for a horizontal bar chart they are the figure's y and x). All null for a trace of another kind.
struct Channels {
  Series* pos{nullptr};
  Series* val{nullptr};
  double* base{nullptr};
  bool horizontal{false};
  explicit operator bool() const noexcept { return pos != nullptr; }
};

[[nodiscard]] inline Channels channels(Trace& trace) {
  auto& payload = Access::payload(trace);
  if (auto* d = std::get_if<TrendData>(&payload)) return {&d->x, &d->y, nullptr, false};
  if (auto* d = std::get_if<CloudData>(&payload)) return {&d->x, &d->y, nullptr, false};
  if (auto* d = std::get_if<RibbonData>(&payload)) return {&d->x, &d->y, &d->base, false};
  if (auto* d = std::get_if<SpikeData>(&payload)) return {&d->x, &d->y, &d->base, false};
  if (auto* d = std::get_if<ColumnsData>(&payload)) {
    return d->horizontal ? Channels{&d->y, &d->x, &d->base, true} : Channels{&d->x, &d->y, &d->base, false};
  }
  return {};
}

[[nodiscard]] inline bool has_channels(const Trace& trace) {
  switch (trace.kind()) {
    case TraceKind::Trend:
    case TraceKind::Cloud:
    case TraceKind::Ribbon:
    case TraceKind::Spike:
    case TraceKind::Columns:
      return true;
    default:
      return false;
  }
}

// True for a trace whose x and y are plain vectors (not the matrix of a stacked ribbon, say).
[[nodiscard]] inline bool is_vector_data(const Trace& trace) {
  return std::visit(
      [](const auto& d) {
        using D = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<D, TrendData> || std::is_same_v<D, CloudData> || std::is_same_v<D, RibbonData> ||
                      std::is_same_v<D, SpikeData> || std::is_same_v<D, ColumnsData>) {
          return d.x.dim() <= 1 && d.y.dim() <= 1 && d.x.numel() == d.y.numel();
        } else {
          return false;
        }
      },
      trace.payload());
}

// A trace the figure itself carries (not one a view drew) that has x/y data a view can work on.
[[nodiscard]] inline bool is_series(const Trace& trace) {
  return trace.visible() && !Access::derived(trace) && has_channels(trace) && is_vector_data(trace);
}

// A series the Focus view has faded: later views that annotate or fit leave it out.
[[nodiscard]] inline bool is_dimmed(const Trace& trace) {
  return trace.stroke_explicit() && trace.stroke_color().alpha < 0.3;
}

[[nodiscard]] inline bool is_horizontal(const Trace& trace) {
  const auto* d = std::get_if<ColumnsData>(&trace.payload());
  return d != nullptr && d->horizontal;
}

// The traces of `pane` a view acts on.
[[nodiscard]] inline std::vector<Trace*> series_of(Pane& pane) {
  std::vector<Trace*> out;
  for (Trace& trace : Access::traces(pane)) {
    if (is_series(trace)) {
      out.push_back(&trace);
    }
  }
  return out;
}

[[nodiscard]] inline std::vector<const Trace*> series_of(const Pane& pane) {
  std::vector<const Trace*> out;
  for (const Trace& trace : pane.traces()) {
    if (is_series(trace)) {
      out.push_back(&trace);
    }
  }
  return out;
}

// Number of points of a series-like trace.
[[nodiscard]] inline std::int64_t point_count(const Trace& trace) {
  return std::visit(
      [](const auto& d) -> std::int64_t {
        using D = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<D, TrendData> || std::is_same_v<D, CloudData> || std::is_same_v<D, RibbonData> ||
                      std::is_same_v<D, SpikeData> || std::is_same_v<D, ColumnsData>) {
          return d.y.numel();
        } else {
          return 0;
        }
      },
      trace.payload());
}

// The colour a trace is drawn in: its own when set, otherwise the palette's sample the renderer uses for its kind.
[[nodiscard]] inline Rgba drawn_color(const Trace& trace) {
  if (trace.stroke_explicit()) {
    return trace.stroke_color();
  }
  switch (trace.kind()) {
    case TraceKind::Ribbon:
      return trace.palette_ramp().sample(0.58);
    case TraceKind::Columns:
      return trace.palette_ramp().sample(0.62);
    case TraceKind::Bucket:
      return trace.palette_ramp().sample(0.55);
    case TraceKind::Quartet:
    case TraceKind::Violin:
      return trace.palette_ramp().sample(0.42);
    case TraceKind::Trend:
    case TraceKind::Cloud:
    case TraceKind::Spike:
    default:
      return trace.stroke_color();
  }
}

[[nodiscard]] inline std::string series_name(const Trace& trace, std::size_t index) {
  return trace.label_text().empty() ? "Series " + std::to_string(index + 1) : trace.label_text();
}

[[nodiscard]] inline std::string number_text(double value) {
  char buffer[48];
  const double magnitude = std::abs(value);
  if (magnitude != 0.0 && (magnitude >= 1e6 || magnitude < 1e-3)) {
    std::snprintf(buffer, sizeof buffer, "%.3g", value);
  } else if (magnitude >= 100.0) {
    std::snprintf(buffer, sizeof buffer, "%.0f", value);
  } else {
    std::snprintf(buffer, sizeof buffer, "%.3g", value);
  }
  return buffer;
}

struct Summary {
  std::size_t count{0};
  double mean{0.0};
  double sd{0.0};
  double min{0.0};
  double max{0.0};
  double q1{0.0};
  double median{0.0};
  double q3{0.0};
};

[[nodiscard]] inline double quantile_of(const Vec& sorted, double q) {
  if (sorted.empty()) {
    return kNaN;
  }
  const double at = q * static_cast<double>(sorted.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(at));
  const auto hi = std::min(lo + 1, sorted.size() - 1);
  return sorted[lo] + (at - static_cast<double>(lo)) * (sorted[hi] - sorted[lo]);
}

[[nodiscard]] inline Summary summarize(const Vec& values) {
  Vec finite;
  finite.reserve(values.size());
  for (const double v : values) {
    if (std::isfinite(v)) {
      finite.push_back(v);
    }
  }
  Summary out;
  out.count = finite.size();
  if (finite.empty()) {
    return out;
  }
  std::sort(finite.begin(), finite.end());
  double sum = 0.0;
  for (const double v : finite) {
    sum += v;
  }
  out.mean = sum / static_cast<double>(finite.size());
  double ss = 0.0;
  for (const double v : finite) {
    ss += (v - out.mean) * (v - out.mean);
  }
  out.sd = finite.size() > 1 ? std::sqrt(ss / static_cast<double>(finite.size() - 1)) : 0.0;
  out.min = finite.front();
  out.max = finite.back();
  out.q1 = quantile_of(finite, 0.25);
  out.median = quantile_of(finite, 0.5);
  out.q3 = quantile_of(finite, 0.75);
  return out;
}

// A trace drawn by a view on top of the figure (a fit, markers, an outlier overlay): never reshaped by later views.
[[nodiscard]] inline Trace make_derived(TraceKind kind, TracePayload payload, std::string label, Rgba color, float width,
                                        bool behind = false) {
  Trace made(kind, std::move(payload));
  made.label(std::move(label));
  made.stroke(color);
  made.width(width);
  Access::mark_derived(made, behind);
  return made;
}

inline Trace& add_derived(Pane& pane, TraceKind kind, TracePayload payload, std::string label, Rgba color, float width) {
  auto& traces = Access::traces(pane);
  traces.push_back(make_derived(kind, std::move(payload), std::move(label), color, width));
  return traces.back();
}

[[nodiscard]] inline Rgba with_alpha(Rgba color, double alpha) {
  color.alpha = alpha;
  return color;
}

[[nodiscard]] inline Rgba shade_of(Rgba color, double factor) {
  return Rgba{std::clamp(color.red * factor, 0.0, 1.0), std::clamp(color.green * factor, 0.0, 1.0),
              std::clamp(color.blue * factor, 0.0, 1.0), color.alpha};
}

[[nodiscard]] inline Rgba mix_color(Rgba a, Rgba b, double t) {
  return Rgba{a.red + (b.red - a.red) * t, a.green + (b.green - a.green) * t, a.blue + (b.blue - a.blue) * t,
              a.alpha + (b.alpha - a.alpha) * t};
}

// `color` made legible as text or a thin line over the pane's background: darker on a light one, lighter on a dark one.
[[nodiscard]] inline Rgba readable_on(const Pane& pane, Rgba color) {
  const Rgba bg = pane.theme_tokens().background();
  const double luma = 0.299 * bg.red + 0.587 * bg.green + 0.114 * bg.blue;
  Rgba out = luma < 0.45 ? mix_color(color, Rgba{1.0, 1.0, 1.0, 1.0}, 0.3) : shade_of(color, 0.62);
  out.alpha = 1.0;
  return out;
}

// One group of the window's view menus: how it lists its options for a figure (empty = does not apply), and how a
// chosen option rewrites a copy of the figure.
struct Def {
  int stage;  // the order groups are applied in (and listed in within a section)
  std::string id;
  std::string title;
  std::string section;
  std::function<std::vector<Option>(const Studio&)> options;
  std::function<void(Studio&, std::string_view option, const Choices& all)> apply;
};

// Keeps the points of a series-like trace that `keep` flags (the other point-wise channels follow).
void filter_points(Trace& trace, const std::vector<bool>& keep);

void register_look_views(std::vector<Def>& defs);      // theme, colors, legend, grid, aspect, text
void register_series_views(std::vector<Def>& defs);    // focus, isolate, window, layout
void register_data_views(std::vector<Def>& defs);      // sort, orient, transform, smooth, draw
void register_analysis_views(std::vector<Def>& defs);  // fit, guides, marks, stats, outliers, shade, points
void register_histogram_views(std::vector<Def>& defs); // bins, counts, curve
void register_axis_views(std::vector<Def>& defs);      // scale, range, width

} // namespace pluss::pl::views::internal
