#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "plot/modules/projector_3d.h"
#include "plot/modules/style.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

enum class Encoding4DStrategy : std::uint8_t {
  Color = 0,
  Slice = 1,
  Facet = 2,
  Frame = 3,
  Size = 4,
};

struct Encoding4D {
  Encoding4DStrategy strategy{Encoding4DStrategy::Color};
  std::int32_t bins{4};
  std::string title{"w"};
  std::string prefix{"slice"};
};

namespace detail {

inline std::pair<double, double> finite_minmax_4d(const std::vector<double>& values) {
  double lo = std::numeric_limits<double>::infinity();
  double hi = -std::numeric_limits<double>::infinity();
  for (double value : values) {
    if (!std::isfinite(value)) {
      continue;
    }
    lo = std::min(lo, value);
    hi = std::max(hi, value);
  }
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    return {0.0, 1.0};
  }
  return {lo, hi};
}

inline std::vector<double> collect_w_values_4d(const std::vector<Trace4D>& traces) {
  std::vector<double> values;
  for (const auto& trace : traces) {
    values.insert(values.end(), trace.w.begin(), trace.w.end());
  }
  return values;
}

inline std::string default_trace_color_4d(std::size_t index) {
  static const std::vector<std::string> palette{
      "#0f766e", "#c2410c", "#1d4ed8", "#b91c1c", "#4d7c0f",
      "#0369a1", "#b45309", "#0f172a", "#be123c", "#57534e"};
  return palette[index % palette.size()];
}

inline std::vector<Trace3D> encode_color_4d(const std::vector<Trace4D>& traces, const Encoding4D& encoding) {
  const auto w_values = collect_w_values_4d(traces);
  const auto [wmin, wmax] = finite_minmax_4d(w_values);
  const auto bins = std::max<std::int32_t>(1, encoding.bins);

  std::vector<Trace3D> out;
  for (std::size_t trace_index = 0; trace_index < traces.size(); ++trace_index) {
    const auto& trace = traces[trace_index];
    const std::size_t n = std::min({trace.x.size(), trace.y.size(), trace.z.size(), trace.w.size()});
    if (n == 0) {
      continue;
    }

    std::vector<std::vector<double>> xs(static_cast<std::size_t>(bins));
    std::vector<std::vector<double>> ys(static_cast<std::size_t>(bins));
    std::vector<std::vector<double>> zs(static_cast<std::size_t>(bins));
    for (std::size_t i = 0; i < n; ++i) {
      const double w = trace.w[i];
      const double t = (wmax > wmin && std::isfinite(w)) ? std::clamp((w - wmin) / (wmax - wmin), 0.0, 1.0) : 0.5;
      const auto idx = static_cast<std::size_t>(std::min<std::int32_t>(
          bins - 1, static_cast<std::int32_t>(std::floor(t * static_cast<double>(bins)))));
      xs[idx].push_back(trace.x[i]);
      ys[idx].push_back(trace.y[i]);
      zs[idx].push_back(trace.z[i]);
    }

    for (std::int32_t bin = 0; bin < bins; ++bin) {
      if (xs[static_cast<std::size_t>(bin)].empty()) {
        continue;
      }
      const double lo = wmin + (wmax - wmin) * static_cast<double>(bin) / static_cast<double>(bins);
      const double hi = wmin + (wmax - wmin) * static_cast<double>(bin + 1) / static_cast<double>(bins);
      Style style = trace.style;
      style.color = surface_color_3d(0.5 * (lo + hi), wmin, wmax);
      if (style.color.empty()) {
        style.color = default_trace_color_4d(trace_index);
      }
      if (trace.kind == Trace4DKind::Scatter4 && style.marker == "none") {
        style.marker = "circle";
      }
      out.push_back(Trace3D{
          .kind = trace.kind == Trace4DKind::Line4 ? Trace3DKind::Line3 : Trace3DKind::Scatter3,
          .name = fmt::format("{} [{:.3g}, {:.3g}]", trace.name.empty() ? std::string("trace4d") : trace.name, lo, hi),
          .x = std::move(xs[static_cast<std::size_t>(bin)]),
          .y = std::move(ys[static_cast<std::size_t>(bin)]),
          .z = std::move(zs[static_cast<std::size_t>(bin)]),
          .style = std::move(style),
      });
    }
  }
  return out;
}

inline std::vector<Trace3D> encode_size_4d(const std::vector<Trace4D>& traces, const Encoding4D& encoding) {
  const auto w_values = collect_w_values_4d(traces);
  const auto [wmin, wmax] = finite_minmax_4d(w_values);
  const auto bins = std::max<std::int32_t>(1, encoding.bins);

  std::vector<Trace3D> out;
  for (std::size_t trace_index = 0; trace_index < traces.size(); ++trace_index) {
    const auto& trace = traces[trace_index];
    const std::size_t n = std::min({trace.x.size(), trace.y.size(), trace.z.size(), trace.w.size()});
    if (n == 0) {
      continue;
    }

    std::vector<std::vector<double>> xs(static_cast<std::size_t>(bins));
    std::vector<std::vector<double>> ys(static_cast<std::size_t>(bins));
    std::vector<std::vector<double>> zs(static_cast<std::size_t>(bins));
    for (std::size_t i = 0; i < n; ++i) {
      const double w = trace.w[i];
      const double t = (wmax > wmin && std::isfinite(w)) ? std::clamp((w - wmin) / (wmax - wmin), 0.0, 1.0) : 0.5;
      const auto idx = static_cast<std::size_t>(std::min<std::int32_t>(
          bins - 1, static_cast<std::int32_t>(std::floor(t * static_cast<double>(bins)))));
      xs[idx].push_back(trace.x[i]);
      ys[idx].push_back(trace.y[i]);
      zs[idx].push_back(trace.z[i]);
    }

    for (std::int32_t bin = 0; bin < bins; ++bin) {
      if (xs[static_cast<std::size_t>(bin)].empty()) {
        continue;
      }
      const double lo = wmin + (wmax - wmin) * static_cast<double>(bin) / static_cast<double>(bins);
      const double hi = wmin + (wmax - wmin) * static_cast<double>(bin + 1) / static_cast<double>(bins);
      const double t = bins == 1 ? 0.5 : static_cast<double>(bin) / static_cast<double>(bins - 1);
      Style style = trace.style;
      if (style.color.empty()) {
        style.color = default_trace_color_4d(trace_index);
      }
      style.line_width = std::max(style.line_width, 1.2) + 2.0 * t;
      style.marker_size = 2.8 + 4.4 * t;
      if (trace.kind == Trace4DKind::Scatter4 && style.marker == "none") {
        style.marker = "circle";
      }
      out.push_back(Trace3D{
          .kind = trace.kind == Trace4DKind::Line4 ? Trace3DKind::Line3 : Trace3DKind::Scatter3,
          .name = fmt::format("{} [{:.3g}, {:.3g}]",
                              trace.name.empty() ? std::string("trace4d") : trace.name,
                              lo,
                              hi),
          .x = std::move(xs[static_cast<std::size_t>(bin)]),
          .y = std::move(ys[static_cast<std::size_t>(bin)]),
          .z = std::move(zs[static_cast<std::size_t>(bin)]),
          .style = std::move(style),
      });
    }
  }
  return out;
}

inline std::vector<std::vector<Trace3D>> encode_slices_4d(const std::vector<Trace4D>& traces, const Encoding4D& encoding) {
  const auto w_values = collect_w_values_4d(traces);
  const auto [wmin, wmax] = finite_minmax_4d(w_values);
  const auto bins = std::max<std::int32_t>(1, encoding.bins);

  std::vector<std::vector<Trace3D>> slices(static_cast<std::size_t>(bins));
  for (std::size_t trace_index = 0; trace_index < traces.size(); ++trace_index) {
    const auto& trace = traces[trace_index];
    const std::size_t n = std::min({trace.x.size(), trace.y.size(), trace.z.size(), trace.w.size()});
    for (std::int32_t bin = 0; bin < bins; ++bin) {
      std::vector<double> x;
      std::vector<double> y;
      std::vector<double> z;
      const double lo = wmin + (wmax - wmin) * static_cast<double>(bin) / static_cast<double>(bins);
      const double hi = wmin + (wmax - wmin) * static_cast<double>(bin + 1) / static_cast<double>(bins);
      for (std::size_t i = 0; i < n; ++i) {
        const double w = trace.w[i];
        if (!std::isfinite(w)) {
          continue;
        }
        const bool in_bin = (bin + 1 == bins) ? (w >= lo && w <= hi) : (w >= lo && w < hi);
        if (!in_bin) {
          continue;
        }
        x.push_back(trace.x[i]);
        y.push_back(trace.y[i]);
        z.push_back(trace.z[i]);
      }
      if (x.empty()) {
        continue;
      }
      Style style = trace.style;
      if (style.color.empty()) {
        style.color = default_trace_color_4d(trace_index);
      }
      if (trace.kind == Trace4DKind::Scatter4 && style.marker == "none") {
        style.marker = "circle";
      }
      slices[static_cast<std::size_t>(bin)].push_back(Trace3D{
          .kind = trace.kind == Trace4DKind::Line4 ? Trace3DKind::Line3 : Trace3DKind::Scatter3,
          .name = trace.name,
          .x = std::move(x),
          .y = std::move(y),
          .z = std::move(z),
          .style = std::move(style),
      });
    }
  }
  return slices;
}

} // namespace detail
} // namespace pluss::plotism
