#include "table/plot/DataFramePlot.h"
#include "table/plot/DataFramePlotOps.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

using Studio = pluss::pl::Studio;
using Pane = pluss::pl::Pane;
using Trace = pluss::pl::Trace;
using Rgba = pluss::pl::Rgba;
namespace flash = pluss::pl::flash;

constexpr std::string_view kDefaultPlotName = "dataframe";

void setup_pane(Pane& pane, bool show_legend) {
  pane.grid(pluss::pl::GridStyle::Major);
  pane.legend(show_legend ? pluss::pl::KeyPlace::Auto : pluss::pl::KeyPlace::None);
}

Studio make_studio() {
  auto fig = pluss::pl::studio(std::string(kDefaultPlotName));
  fig.theme(pluss::pl::themes::paper());
  return fig;
}

Rgba color_for_index(std::size_t index) {
  static const std::array<Rgba, 10> palette{{
      Rgba::from_u8(15, 118, 110),
      Rgba::from_u8(194, 65, 12),
      Rgba::from_u8(29, 78, 216),
      Rgba::from_u8(185, 28, 28),
      Rgba::from_u8(77, 124, 15),
      Rgba::from_u8(3, 105, 161),
      Rgba::from_u8(180, 83, 9),
      Rgba::from_u8(15, 23, 42),
      Rgba::from_u8(190, 18, 60),
      Rgba::from_u8(87, 83, 78),
  }};
  return palette[index % palette.size()];
}

void style_trace(Trace& trace,
                 std::size_t index,
                 double alpha = 1.0,
                 double line_width = 1.5) {
  auto color = color_for_index(index);
  color.alpha = std::clamp(alpha, 0.05, 1.0);
  trace.stroke(color);
  trace.width(static_cast<float>(line_width));
  if (color.alpha < 0.999) {
    trace.transparent(true);
  }
}

std::vector<double> positional_plot_x(std::int64_t nrows) {
  std::vector<double> out(static_cast<std::size_t>(nrows), 0.0);
  for (std::int64_t i = 0; i < nrows; ++i) {
    out[static_cast<std::size_t>(i)] = static_cast<double>(i);
  }
  return out;
}

std::vector<double> thread_to_plot_numeric_values(const Thread& t) {
  std::vector<double> out;
  switch (t.kind()) {
    case ThreadKind::Series: {
      out = t.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
      break;
    }
    case ThreadKind::Timeline: {
      const auto ticks = t.as_timeline().to_epoch_nanos_vector();
      out.reserve(ticks.size());
      for (const auto tick : ticks) {
        out.push_back(static_cast<double>(tick));
      }
      break;
    }
    case ThreadKind::TimeIndex: {
      const auto ticks = t.as_timeindex().timeline().to_epoch_nanos_vector();
      out.reserve(ticks.size());
      for (const auto tick : ticks) {
        out.push_back(static_cast<double>(tick));
      }
      break;
    }
    case ThreadKind::Note: {
      const auto& note = t.as_note();
      out.reserve(static_cast<std::size_t>(note.numel()));
      note.for_each_flat([&](std::string_view s) {
        std::string value(s);
        char* end = nullptr;
        const double parsed = std::strtod(value.c_str(), &end);
        const bool valid = (end != nullptr) && (end != value.c_str()) && (*end == '\0');
        out.push_back(valid ? parsed : std::numeric_limits<double>::quiet_NaN());
      });
      break;
    }
    default:
      PLUSS_FAIL("DataFrame::plot: unsupported Thread kind for numeric plotting");
  }
  return out;
}

std::vector<std::string> thread_to_plot_labels(const Thread& t) {
  const auto note_like = (t.kind() == ThreadKind::Note) ? t : t.cast(ThreadCastTarget::Note);
  std::vector<std::string> out;
  const auto& note = note_like.as_note();
  out.reserve(static_cast<std::size_t>(note.numel()));
  note.for_each_flat([&](std::string_view s) { out.emplace_back(s); });
  return out;
}

void require_plot_size(const std::vector<double>& values, std::int64_t expected, std::string_view feature_name) {
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == expected,
              "DataFrame::plot: feature '" + std::string(feature_name) + "' length mismatch with cases");
}

void require_plot_size(const std::vector<std::string>& values,
                       std::int64_t expected,
                       std::string_view feature_name,
                       std::string_view where) {
  PLUSS_CHECK(static_cast<std::int64_t>(values.size()) == expected,
              std::string(where) + ": feature '" + std::string(feature_name) + "' length mismatch with cases");
}

bool thread_prefers_labeled_axis(const Thread& t) {
  return t.kind() == ThreadKind::Note || t.kind() == ThreadKind::Timeline || t.kind() == ThreadKind::TimeIndex;
}

struct ResolvedPlotAxis {
  std::vector<double> values{};
  std::vector<std::string> labels{};
  std::string label{"case"};
};

std::pair<double, double> cell_bounds(std::span<const double> coords, std::size_t index) {
  if (coords.empty() || index >= coords.size()) {
    return {0.0, 1.0};
  }
  if (coords.size() == 1) {
    return {coords.front() - 0.5, coords.front() + 0.5};
  }
  const double center = coords[index];
  const double lo = (index == 0) ? center - 0.5 * (coords[1] - coords[0]) : 0.5 * (coords[index - 1] + center);
  const double hi =
      (index + 1 == coords.size()) ? center + 0.5 * (center - coords[index - 1]) : 0.5 * (center + coords[index + 1]);
  return {lo, hi};
}

ResolvedPlotAxis resolve_plot_axis(const DataFrame& frame,
                                   std::optional<std::string_view> x_feature,
                                   std::string_view where) {
  const auto nrows = frame.nrows();
  const auto axis_name = x_feature.has_value() ? std::string(*x_feature) : std::string("case");
  const auto axis_thread = x_feature.has_value() ? frame.features(*x_feature) : frame.cases();

  ResolvedPlotAxis axis;
  axis.label = axis_name;
  if (thread_prefers_labeled_axis(axis_thread)) {
    axis.values = positional_plot_x(nrows);
    axis.labels = thread_to_plot_labels(axis_thread);
    require_plot_size(axis.labels, nrows, axis_name, where);
    return axis;
  }

  axis.values = thread_to_plot_numeric_values(axis_thread);
  require_plot_size(axis.values, nrows, axis_name);
  return axis;
}

void apply_axis(Pane& pane, const ResolvedPlotAxis& axis) {
  pane.x_label(axis.label);
  if (!axis.labels.empty()) {
    pane.x_ticks(axis.values, axis.labels);
  }
}

std::vector<std::string> labels_for_cases(const DataFrame& frame, std::string_view where) {
  auto labels = thread_to_plot_labels(frame.cases());
  require_plot_size(labels, frame.nrows(), "case", where);
  return labels;
}

bool thread_supports_correlation(const Thread& thread) {
  return thread.kind() == ThreadKind::Series;
}

bool thread_supports_hue(const Thread& thread) {
  return thread.kind() == ThreadKind::Note || thread.kind() == ThreadKind::Timeline || thread.kind() == ThreadKind::TimeIndex;
}

std::vector<std::string> numeric_feature_names(const DataFrame& frame,
                                               std::initializer_list<std::string_view> requested,
                                               std::string_view where);

std::vector<std::string> hue_feature_names(const DataFrame& frame) {
  std::vector<std::string> names;
  for (const auto& feature : frame.feature_names()) {
    if (thread_supports_hue(frame.features(feature))) {
      names.push_back(feature);
    }
  }
  return names;
}

std::vector<std::string> correlation_feature_names(const DataFrame& frame,
                                                   std::initializer_list<std::string_view> requested) {
  return numeric_feature_names(frame, requested, "DataFrame::plot().corr_heatmap");
}

std::vector<std::string> numeric_feature_names(const DataFrame& frame,
                                               std::initializer_list<std::string_view> requested,
                                               std::string_view where) {
  std::vector<std::string> names;
  if (requested.size() > 0) {
    names.reserve(requested.size());
    for (const auto feature : requested) {
      const auto thread = frame.features(feature);
      PLUSS_CHECK(thread_supports_correlation(thread),
                  std::string(where) + ": feature '" + std::string(feature) + "' is not a numeric Series");
      names.emplace_back(feature);
    }
    return names;
  }

  for (const auto& feature : frame.feature_names()) {
    if (thread_supports_correlation(frame.features(feature))) {
      names.push_back(feature);
    }
  }
  PLUSS_CHECK(!names.empty(), std::string(where) + ": no numeric Series features available");
  return names;
}

double pairwise_correlation(const std::vector<double>& lhs, const std::vector<double>& rhs) {
  const std::size_t n = std::min(lhs.size(), rhs.size());
  double sum_x = 0.0;
  double sum_y = 0.0;
  double sum_xx = 0.0;
  double sum_yy = 0.0;
  double sum_xy = 0.0;
  std::size_t count = 0;

  for (std::size_t i = 0; i < n; ++i) {
    const double x = lhs[i];
    const double y = rhs[i];
    if (!std::isfinite(x) || !std::isfinite(y)) {
      continue;
    }
    sum_x += x;
    sum_y += y;
    sum_xx += x * x;
    sum_yy += y * y;
    sum_xy += x * y;
    ++count;
  }

  if (count < 2) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const double n_d = static_cast<double>(count);
  const double cov = sum_xy - (sum_x * sum_y / n_d);
  const double var_x = sum_xx - (sum_x * sum_x / n_d);
  const double var_y = sum_yy - (sum_y * sum_y / n_d);
  if (!(var_x > 0.0) || !(var_y > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return cov / std::sqrt(var_x * var_y);
}

double quantile_sorted(const std::vector<double>& sorted, double q) {
  if (sorted.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (sorted.size() == 1) {
    return sorted.front();
  }
  const double clamped = std::clamp(q, 0.0, 1.0);
  const double position = clamped * static_cast<double>(sorted.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(position));
  const auto hi = static_cast<std::size_t>(std::ceil(position));
  if (lo == hi) {
    return sorted[lo];
  }
  const double weight = position - static_cast<double>(lo);
  return sorted[lo] * (1.0 - weight) + sorted[hi] * weight;
}

struct BoxSummary {
  double q1{0.0};
  double median{0.0};
  double q3{0.0};
  double whisker_low{0.0};
  double whisker_high{0.0};
  std::vector<double> outliers{};
};

BoxSummary summarize_boxplot(const std::vector<double>& values) {
  std::vector<double> finite;
  finite.reserve(values.size());
  for (double value : values) {
    if (std::isfinite(value)) {
      finite.push_back(value);
    }
  }
  PLUSS_CHECK(!finite.empty(), "DataFrame::plot().box: feature contains no finite values");
  std::sort(finite.begin(), finite.end());

  BoxSummary summary;
  summary.q1 = quantile_sorted(finite, 0.25);
  summary.median = quantile_sorted(finite, 0.5);
  summary.q3 = quantile_sorted(finite, 0.75);
  const double iqr = summary.q3 - summary.q1;
  const double lo_fence = summary.q1 - 1.5 * iqr;
  const double hi_fence = summary.q3 + 1.5 * iqr;

  summary.whisker_low = finite.front();
  summary.whisker_high = finite.back();
  for (double value : finite) {
    if (value >= lo_fence) {
      summary.whisker_low = value;
      break;
    }
  }
  for (auto it = finite.rbegin(); it != finite.rend(); ++it) {
    if (*it <= hi_fence) {
      summary.whisker_high = *it;
      break;
    }
  }
  for (double value : finite) {
    if (value < summary.whisker_low || value > summary.whisker_high) {
      summary.outliers.push_back(value);
    }
  }
  return summary;
}

bool all_nonnegative_columns(const DataFrame& frame, const std::vector<std::string>& names) {
  for (const auto& name : names) {
    const auto values = thread_to_plot_numeric_values(frame.features(name));
    for (double value : values) {
      if (std::isfinite(value) && value < 0.0) {
        return false;
      }
    }
  }
  return true;
}

bool prefer_index_plot(const DataFrame& frame) {
  return thread_prefers_labeled_axis(frame.cases()) || frame.nrows() > 24;
}

Trace& add_heatmap(Pane& pane, const std::vector<std::vector<double>>& matrix, std::string label) {
  const std::int64_t rows = static_cast<std::int64_t>(matrix.size());
  const std::int64_t cols = rows == 0 ? 0 : static_cast<std::int64_t>(matrix.front().size());
  std::vector<double> z_values(static_cast<std::size_t>(std::max<std::int64_t>(0, rows * cols)),
                               std::numeric_limits<double>::quiet_NaN());
  for (std::int64_t r = 0; r < rows; ++r) {
    const auto& row = matrix[static_cast<std::size_t>(r)];
    for (std::int64_t c = 0; c < cols && c < static_cast<std::int64_t>(row.size()); ++c) {
      z_values[static_cast<std::size_t>(r * cols + c)] = row[static_cast<std::size_t>(c)];
    }
  }

  std::vector<double> x(static_cast<std::size_t>(std::max<std::int64_t>(0, cols)), 0.0);
  std::vector<double> y(static_cast<std::size_t>(std::max<std::int64_t>(0, rows)), 0.0);
  for (std::int64_t c = 0; c < cols; ++c) {
    x[static_cast<std::size_t>(c)] = static_cast<double>(c);
  }
  for (std::int64_t r = 0; r < rows; ++r) {
    y[static_cast<std::size_t>(r)] = static_cast<double>(r);
  }

  auto z = pluss::make_series(z_values, {rows * cols});
  z = z.reshape({rows, cols});
  auto& trace = pane.fieldmap(x, y, z);
  trace.label(std::move(label));
  trace.shade(pluss::pl::ShadeMode::Smooth);
  return trace;
}

void configure_heatmap(Pane& pane, Trace& trace, bool annotate_levels, bool scale_guide) {
  trace.annotate_levels(annotate_levels);
  pane.scale_guide(scale_guide);
}

std::pair<std::vector<double>, std::vector<double>> make_step_xy(const std::vector<double>& x,
                                                                  const std::vector<double>& y) {
  const std::size_t n = std::min(x.size(), y.size());
  std::vector<double> sx;
  std::vector<double> sy;
  if (n == 0) {
    return {std::move(sx), std::move(sy)};
  }
  sx.reserve(n * 2 - 1);
  sy.reserve(n * 2 - 1);
  for (std::size_t i = 0; i < n; ++i) {
    sx.push_back(x[i]);
    sy.push_back(y[i]);
    if (i + 1 < n) {
      sx.push_back(x[i + 1]);
      sy.push_back(y[i]);
    }
  }
  return {std::move(sx), std::move(sy)};
}

Studio corr_heatmap_from_names(const DataFrame& frame,
                               const std::vector<std::string>& names,
                               bool annotate_levels,
                               bool scale_guide) {
  std::vector<std::vector<double>> columns;
  columns.reserve(names.size());
  for (const auto& name : names) {
    auto values = thread_to_plot_numeric_values(frame.features(name));
    require_plot_size(values, frame.nrows(), name);
    columns.push_back(std::move(values));
  }

  std::vector<std::vector<double>> matrix(names.size(), std::vector<double>(names.size(), 0.0));
  for (std::size_t r = 0; r < names.size(); ++r) {
    for (std::size_t c = 0; c < names.size(); ++c) {
      matrix[r][c] = (r == c) ? 1.0 : pairwise_correlation(columns[r], columns[c]);
    }
  }

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = add_heatmap(pane, matrix, "corr");
  configure_heatmap(pane, trace, annotate_levels, scale_guide);

  const auto tick_pos = positional_plot_x(static_cast<std::int64_t>(names.size()));
  pane.x_ticks(tick_pos, names).y_ticks(tick_pos, names);
  pane.x_label("feature").y_label("feature");
  return fig;
}

Studio scatter_matrix_from_names(const DataFrame& frame,
                                 const std::vector<std::string>& names,
                                 ScatterMatrixTriangle triangle,
                                 bool monochrome,
                                 std::optional<int> auto_tick_decimals) {
  PLUSS_CHECK(names.size() >= 2, "DataFrame::plot().scatter_matrix: need at least two numeric features");

  std::vector<std::vector<double>> columns;
  columns.reserve(names.size());
  for (const auto& name : names) {
    auto values = thread_to_plot_numeric_values(frame.features(name));
    require_plot_size(values, frame.nrows(), name);
    columns.push_back(std::move(values));
  }

  auto axis_range = [](const std::vector<double>& values) -> pluss::pl::Range1d {
    double lo = std::numeric_limits<double>::infinity();
    double hi = -std::numeric_limits<double>::infinity();
    for (const double value : values) {
      if (!std::isfinite(value)) {
        continue;
      }
      lo = std::min(lo, value);
      hi = std::max(hi, value);
    }
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
      return {0.0, 1.0};
    }
    if (!(hi > lo)) {
      const double pad = std::max(0.5, std::abs(lo) * 0.05);
      return {lo - pad, hi + pad};
    }
    const double pad = (hi - lo) * 0.04;
    return {lo - pad, hi + pad};
  };

  std::vector<pluss::pl::Range1d> feature_ranges;
  feature_ranges.reserve(columns.size());
  for (const auto& values : columns) {
    feature_ranges.push_back(axis_range(values));
  }

  const auto dims = static_cast<std::int64_t>(names.size());
  auto fig = make_studio();
  fig.mosaic(dims, dims, 12.0, 12.0);
  for (std::int64_t row = 0; row < dims; ++row) {
    for (std::int64_t col = 0; col < dims; ++col) {
      auto& pane = fig.pane2();
      pane.place(row, col);
      const bool keep_cell =
          triangle == ScatterMatrixTriangle::Full ||
          (triangle == ScatterMatrixTriangle::Lower && row >= col) ||
          (triangle == ScatterMatrixTriangle::Upper && row <= col);
      if (!keep_cell) {
        pane.hide();
        continue;
      }

      setup_pane(pane, false);
      if (auto_tick_decimals.has_value()) {
        pane.x_auto_tick_decimals(*auto_tick_decimals);
      }
      pane.x_limits(feature_ranges[static_cast<std::size_t>(col)]);
      if (row == col) {
        auto& trace = flash::histplot(pane, columns[static_cast<std::size_t>(col)], 12);
        trace.label(names[static_cast<std::size_t>(col)]);
        style_trace(trace, monochrome ? 0U : static_cast<std::size_t>(col), 0.9, 1.0);
      } else {
        if (auto_tick_decimals.has_value()) {
          pane.y_auto_tick_decimals(*auto_tick_decimals);
        }
        pane.y_limits(feature_ranges[static_cast<std::size_t>(row)]);
        auto& trace = flash::scatterplot(pane,
                                         columns[static_cast<std::size_t>(col)],
                                         columns[static_cast<std::size_t>(row)]);
        trace.label(names[static_cast<std::size_t>(row)] + " vs " + names[static_cast<std::size_t>(col)]);
        style_trace(trace, monochrome ? 0U : static_cast<std::size_t>(row + col), 0.92, 1.2);
      }
      pane.x_label(row + 1 == dims ? names[static_cast<std::size_t>(col)] : "");
      pane.y_label(col == 0 ? names[static_cast<std::size_t>(row)] : "");
    }
  }
  return fig;
}

Studio box_from_names(const DataFrame& frame, const std::vector<std::string>& names) {
  PLUSS_CHECK(!names.empty(), "DataFrame::plot().box: at least one numeric feature is required");

  std::vector<double> group_id;
  std::vector<double> merged_values;
  group_id.reserve(static_cast<std::size_t>(frame.nrows()) * names.size());
  merged_values.reserve(static_cast<std::size_t>(frame.nrows()) * names.size());

  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = thread_to_plot_numeric_values(frame.features(names[i]));
    require_plot_size(values, frame.nrows(), names[i]);
    const auto summary = summarize_boxplot(values);
    (void)summary;
    for (const double value : values) {
      if (!std::isfinite(value)) {
        continue;
      }
      group_id.push_back(static_cast<double>(i));
      merged_values.push_back(value);
    }
  }

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = flash::boxplot(pane, group_id, merged_values);
  trace.label("distribution");
  style_trace(trace, 0, 0.95, 1.1);

  const auto centers = positional_plot_x(static_cast<std::int64_t>(names.size()));
  pane.x_ticks(centers, names);
  pane.x_label("feature").y_label("value");
  return fig;
}

Studio violin_from_names(const DataFrame& frame, const std::vector<std::string>& names) {
  PLUSS_CHECK(!names.empty(), "DataFrame::plot().violin: at least one numeric feature is required");

  std::vector<double> group_id;
  std::vector<double> merged_values;
  group_id.reserve(static_cast<std::size_t>(frame.nrows()) * names.size());
  merged_values.reserve(static_cast<std::size_t>(frame.nrows()) * names.size());

  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = thread_to_plot_numeric_values(frame.features(names[i]));
    require_plot_size(values, frame.nrows(), names[i]);
    for (const double value : values) {
      if (!std::isfinite(value)) {
        continue;
      }
      group_id.push_back(static_cast<double>(i));
      merged_values.push_back(value);
    }
  }

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = flash::violinplot(pane, group_id, merged_values);
  trace.label("distribution");
  style_trace(trace, 0, 0.92, 1.1);

  const auto centers = positional_plot_x(static_cast<std::int64_t>(names.size()));
  pane.x_ticks(centers, names);
  pane.x_label("feature").y_label("value");
  return fig;
}

template <typename AddTraceFn>
Studio make_xy_figure(const DataFrame& frame,
                      std::initializer_list<std::string_view> y_features,
                      std::optional<std::string_view> x_feature,
                      std::string_view where,
                      AddTraceFn&& add_trace) {
  PLUSS_CHECK(y_features.size() > 0, std::string(where) + ": at least one y feature is required");

  const auto axis = resolve_plot_axis(frame, x_feature, where);
  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1);
  apply_axis(pane, axis);

  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame.features(y_name));
    require_plot_size(y_values, frame.nrows(), y_name);
    add_trace(pane, axis, y_values, y_name, feature_index);
    ++feature_index;
  }
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));
  return fig;
}

std::tuple<std::vector<double>, std::vector<double>, std::vector<std::string>> grouped_bar_layout(
    const ResolvedPlotAxis& axis,
    std::size_t group_index,
    std::size_t group_count) {
  std::vector<double> centers = axis.values;
  std::vector<double> widths(axis.values.size(), 0.8);
  if (group_count == 0) {
    return {std::move(centers), std::move(widths), axis.labels};
  }

  for (std::size_t i = 0; i < axis.values.size(); ++i) {
    const auto [slot_lo, slot_hi] = cell_bounds(axis.values, i);
    const double slot_width = std::max(1e-9, slot_hi - slot_lo);
    const double group_span = slot_width * 0.82;
    const double bar_width = group_span / static_cast<double>(group_count);
    const double bar_center =
        slot_lo + 0.5 * (slot_width - group_span) + bar_width * (static_cast<double>(group_index) + 0.5);
    centers[i] = bar_center;
    widths[i] = bar_width * 0.92;
  }
  return {std::move(centers), std::move(widths), axis.labels};
}

double z_score_for_confidence(double confidence_level) {
  const double clamped = std::clamp(confidence_level, 0.5, 0.999);
  if (clamped >= 0.99) {
    return 2.575829;
  }
  if (clamped >= 0.98) {
    return 2.326348;
  }
  if (clamped >= 0.95) {
    return 1.959964;
  }
  if (clamped >= 0.9) {
    return 1.644854;
  }
  return 1.281552;
}

std::string compact_number_label(double value) {
  const double rounded = std::round(value);
  if (std::abs(value - rounded) < 1e-9) {
    return std::to_string(static_cast<std::int64_t>(rounded));
  }
  std::string text = std::to_string(value);
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

struct AggregatedBarSeries {
  ResolvedPlotAxis axis{};
  std::vector<double> mean{};
  std::vector<double> ci_half_width{};
};

AggregatedBarSeries aggregate_for_bar_ci(const ResolvedPlotAxis& axis,
                                         const std::vector<double>& y_values,
                                         double confidence_level) {
  PLUSS_CHECK(axis.values.size() == y_values.size(),
              "DataFrame::plot().bar(ci): x/y length mismatch");

  const bool has_axis_labels = !axis.labels.empty();
  const auto z = z_score_for_confidence(confidence_level);

  std::unordered_map<std::string, std::size_t> group_index;
  group_index.reserve(axis.values.size());

  std::vector<std::vector<double>> grouped_samples;
  std::vector<double> grouped_x;
  std::vector<std::string> grouped_labels;

  for (std::size_t i = 0; i < axis.values.size(); ++i) {
    const auto& y = y_values[i];
    if (!std::isfinite(y)) {
      continue;
    }

    const std::string label = has_axis_labels ? axis.labels[i] : compact_number_label(axis.values[i]);
    const std::string key = has_axis_labels ? label : std::to_string(axis.values[i]);
    const auto [it, inserted] = group_index.emplace(key, grouped_samples.size());
    if (inserted) {
      grouped_samples.emplace_back();
      grouped_x.push_back(axis.values[i]);
      if (has_axis_labels) {
        grouped_labels.push_back(label);
      }
    }
    grouped_samples[it->second].push_back(y);
  }

  AggregatedBarSeries out;
  out.axis.label = axis.label;
  if (has_axis_labels) {
    out.axis.values = positional_plot_x(static_cast<std::int64_t>(grouped_labels.size()));
    out.axis.labels = grouped_labels;
  } else {
    out.axis.values = grouped_x;
  }

  out.mean.reserve(grouped_samples.size());
  out.ci_half_width.reserve(grouped_samples.size());

  for (const auto& samples : grouped_samples) {
    if (samples.empty()) {
      out.mean.push_back(std::numeric_limits<double>::quiet_NaN());
      out.ci_half_width.push_back(0.0);
      continue;
    }

    const double n = static_cast<double>(samples.size());
    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    const double mean = sum / n;
    out.mean.push_back(mean);

    if (samples.size() < 2) {
      out.ci_half_width.push_back(0.0);
      continue;
    }

    const double sq_sum = std::accumulate(samples.begin(), samples.end(), 0.0,
                                          [mean](double acc, double value) {
                                            const double d = value - mean;
                                            return acc + d * d;
                                          });
    const double var = sq_sum / (n - 1.0);
    const double sem = std::sqrt(var / n);
    out.ci_half_width.push_back(z * sem);
  }

  return out;
}

} // namespace

DataFramePlotAccessor::DataFramePlotAccessor(DataFrame frame, DataFramePlotOptions options)
    : frame_(std::move(frame)), options_(std::move(options)) {}

std::optional<std::string_view> DataFramePlotAccessor::resolve_x_feature(
    std::optional<std::string_view> x_feature) const {
  if (x_feature.has_value()) {
    return x_feature;
  }
  if (options_.default_x_feature.has_value()) {
    return std::string_view(*options_.default_x_feature);
  }
  return std::nullopt;
}

std::optional<std::string_view> DataFramePlotAccessor::resolve_hue_feature(
    std::optional<std::string_view> hue_feature) const {
  if (hue_feature.has_value()) {
    return hue_feature;
  }
  if (options_.default_hue_feature.has_value()) {
    return std::string_view(*options_.default_hue_feature);
  }
  return std::nullopt;
}

std::int64_t DataFramePlotAccessor::resolve_bins(std::int64_t bins) const noexcept {
  const std::int64_t fallback = std::max<std::int64_t>(1, options_.default_bins);
  if (bins <= 0) {
    return fallback;
  }
  if (bins == 20 && options_.default_bins != 20) {
    return fallback;
  }
  return bins;
}

void DataFramePlotAccessor::apply_figure_options(pluss::pl::Studio& figure) const {
  const double width = options_.width.value_or(figure.extent().width);
  const double height = options_.height.value_or(figure.extent().height);
  figure.size(width, height);
  if (options_.use_tight_layout) {
    figure.tight_layout(true, options_.tight_layout_padding);
  }
}

pluss::pl::Studio DataFramePlotAccessor::line(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Line, _ops::PlotCall{{y_feature}, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::line(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Line, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_line(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().line",
                            [](Pane& pane,
                               const ResolvedPlotAxis& axis,
                               const std::vector<double>& y_values,
                               std::string_view name,
                               std::size_t index) {
                              auto& trace = flash::lineplot(pane, axis.values, y_values);
                              trace.label(std::string(name));
                              style_trace(trace, index, 1.0, 1.6);
                            });
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::step(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Step, _ops::PlotCall{{y_feature}, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::step(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Step, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_step(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().step",
                            [](Pane& pane,
                               const ResolvedPlotAxis& axis,
                               const std::vector<double>& y_values,
                               std::string_view name,
                               std::size_t index) {
                              auto [sx, sy] = make_step_xy(axis.values, y_values);
                              auto& trace = pane.trend(sx, sy);
                              trace.label(std::string(name));
                              style_trace(trace, index, 1.0, 1.5);
                            });
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::area(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Area, _ops::PlotCall{{y_feature}, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::area(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Area, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_area(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().area",
                            [](Pane& pane,
                               const ResolvedPlotAxis& axis,
                               const std::vector<double>& y_values,
                               std::string_view name,
                               std::size_t index) {
                              auto& trace = pane.ribbon(axis.values, y_values, 0.0);
                              trace.label(std::string(name));
                              style_trace(trace, index, 0.88, 1.3);
                            });
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::area_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::AreaStacked, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_area_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().area_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().area_stacked");
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1);
  apply_axis(pane, axis);
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  std::vector<double> baselines(static_cast<std::size_t>(frame_.nrows()), 0.0);
  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(heights, frame_.nrows(), y_name);
    std::vector<double> tops(heights.size(), 0.0);
    for (std::size_t i = 0; i < heights.size(); ++i) {
      tops[i] = baselines[i] + heights[i];
    }
    auto& trace = pane.ribbon(axis.values, tops, 0.0);
    trace.label(std::string(y_name));
    style_trace(trace, feature_index, 0.7, 1.2);
    baselines = std::move(tops);
    ++feature_index;
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::scatter(std::string_view x_feature,
                                                 std::string_view y_feature,
                                                 std::optional<std::string_view> hue_feature) const {
  return _ops::call(
      *this,
      _ops::PlotDispatch::Scatter,
      _ops::PlotCall{{x_feature, y_feature}, std::nullopt, hue_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_scatter(std::string_view x_feature,
                                                      std::string_view y_feature,
                                                      std::optional<std::string_view> hue_feature) const {
  const auto resolved_hue = resolve_hue_feature(hue_feature);
  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().scatter");
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, frame_.nrows(), y_feature);

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, resolved_hue.has_value());
  apply_axis(pane, axis);
  pane.y_label(std::string(y_feature));

  if (!resolved_hue.has_value()) {
    auto& trace = flash::scatterplot(pane, axis.values, y_values);
    trace.label(std::string(y_feature));
    style_trace(trace, 0, 0.92, 1.1);
    return fig;
  }

  const auto hue_thread = frame_.features(*resolved_hue);
  PLUSS_CHECK(thread_supports_hue(hue_thread),
              "DataFrame::plot().scatter: hue feature '" + std::string(*resolved_hue) + "' must be note-like or time-like");
  const auto hue_labels = thread_to_plot_labels(hue_thread);
  require_plot_size(hue_labels, frame_.nrows(), *resolved_hue, "DataFrame::plot().scatter");

  std::vector<std::string> groups;
  for (const auto& label : hue_labels) {
    if (std::find(groups.begin(), groups.end(), label) == groups.end()) {
      groups.push_back(label);
    }
  }

  for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
    std::vector<double> grouped_x;
    std::vector<double> grouped_y;
    for (std::size_t i = 0; i < hue_labels.size(); ++i) {
      if (hue_labels[i] != groups[group_index]) {
        continue;
      }
      grouped_x.push_back(axis.values[i]);
      grouped_y.push_back(y_values[i]);
    }
    auto& trace = flash::scatterplot(pane, grouped_x, grouped_y);
    trace.label(groups[group_index]);
    style_trace(trace, group_index, 0.9, 1.1);
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Bar, _ops::PlotCall{{y_feature}, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::Bar, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_bar(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().bar");
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1);
  apply_axis(pane, axis);
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  const std::size_t group_count = y_features.size();
  std::size_t group_index = 0;
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(y_values, frame_.nrows(), y_name);

    AggregatedBarSeries ci_ready{};
    ci_ready.axis = axis;
    ci_ready.mean = y_values;
    ci_ready.ci_half_width.assign(y_values.size(), 0.0);
    if (options_.bar_ci_enabled) {
      ci_ready = aggregate_for_bar_ci(axis, y_values, options_.bar_ci_level);
    }

    auto [centers, widths, labels] = grouped_bar_layout(ci_ready.axis, group_index, group_count);
    (void)widths;
    (void)labels;
    auto& trace = flash::barplot(pane, centers, ci_ready.mean);
    trace.label(std::string(y_name));
    style_trace(trace, group_index, 0.88, 1.0);

    if (options_.bar_ci_enabled) {
      auto& ci = pane.whisk(centers, ci_ready.mean, ci_ready.ci_half_width);
      ci.label(std::string(y_name) + " CI");
      style_trace(ci, group_index, 0.7, 0.95);
    }
    ++group_index;
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::bar_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::BarStacked, _ops::PlotCall{y_features, x_feature});
}

pluss::pl::Studio DataFramePlotAccessor::impl_bar_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().bar_stacked");
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1);
  apply_axis(pane, axis);
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  std::vector<double> baselines(static_cast<std::size_t>(frame_.nrows()), 0.0);
  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(heights, frame_.nrows(), y_name);
    std::vector<double> tops(heights.size(), 0.0);
    for (std::size_t i = 0; i < heights.size(); ++i) {
      tops[i] = baselines[i] + heights[i];
    }
    auto& trace = flash::barplot(pane, axis.values, tops);
    trace.label(std::string(y_name));
    style_trace(trace, feature_index, 0.65, 1.0);
    baselines = std::move(tops);
    ++feature_index;
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::box(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::Box, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::impl_box(std::initializer_list<std::string_view> features) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().box");
  return box_from_names(frame_, names);
}

pluss::pl::Studio DataFramePlotAccessor::violin(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::Violin, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::violin(std::string_view value_feature,
                                                std::string_view group_feature) const {
  return _ops::call(*this, _ops::PlotDispatch::ViolinGrouped, _ops::PlotCall{{group_feature, value_feature}});
}

pluss::pl::Studio DataFramePlotAccessor::impl_violin(std::initializer_list<std::string_view> features) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().violin");
  return violin_from_names(frame_, names);
}

pluss::pl::Studio DataFramePlotAccessor::impl_violin(std::string_view value_feature,
                                                     std::string_view group_feature) const {
  const auto y_values = thread_to_plot_numeric_values(frame_.features(value_feature));
  require_plot_size(y_values, frame_.nrows(), value_feature);
  const auto y_series = pluss::make_series(y_values, {static_cast<std::int64_t>(y_values.size())});

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false);

  const auto group_thread = frame_.features(group_feature);
  if (thread_supports_hue(group_thread)) {
    auto& trace = flash::violinplot(pane, pluss::make_note(thread_to_plot_labels(group_thread)), y_series);
    trace.label(std::string(value_feature));
    style_trace(trace, 0, 0.92, 1.1);
  } else {
    auto group_values = thread_to_plot_numeric_values(group_thread);
    require_plot_size(group_values, frame_.nrows(), group_feature);
    auto& trace = flash::violinplot(pane,
                                    pluss::make_series(group_values, {static_cast<std::int64_t>(group_values.size())}),
                                    y_series);
    trace.label(std::string(value_feature));
    style_trace(trace, 0, 0.92, 1.1);
  }

  pane.x_label(std::string(group_feature)).y_label(std::string(value_feature));
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins) const {
  return _ops::call(*this, _ops::PlotDispatch::Hist, _ops::PlotCall{{feature}, std::nullopt, std::nullopt, bins});
}

pluss::pl::Studio DataFramePlotAccessor::impl_hist(std::string_view feature, std::int64_t bins) const {
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = flash::histplot(pane,
                                thread_to_plot_numeric_values(frame_.features(feature)),
                                static_cast<int>(std::max<std::int64_t>(1, resolve_bins(bins))));
  trace.label(std::string(feature));
  style_trace(trace, 0, 0.95, 1.0);
  pane.x_label(std::string(feature)).y_label("count");
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::Heatmap, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(std::initializer_list<std::string_view> features) const {
  PLUSS_CHECK(features.size() > 0, "DataFrame::plot().heatmap: at least one feature is required");

  std::vector<std::vector<double>> values;
  std::vector<std::string> y_labels;
  values.reserve(features.size());
  y_labels.reserve(features.size());
  for (const auto feature : features) {
    auto feature_values = thread_to_plot_numeric_values(frame_.features(feature));
    require_plot_size(feature_values, frame_.nrows(), feature);
    values.push_back(std::move(feature_values));
    y_labels.emplace_back(feature);
  }

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = add_heatmap(pane, values, "value");
  configure_heatmap(pane, trace, options_.heatmap_annotate_levels, options_.heatmap_scale_guide);
  pane.x_ticks(positional_plot_x(frame_.nrows()), labels_for_cases(frame_, "DataFrame::plot().heatmap"));
  pane.y_ticks(positional_plot_x(static_cast<std::int64_t>(y_labels.size())), y_labels);
  pane.x_label("case").y_label("feature");
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(std::string_view case_feature,
                                                 std::string_view feature_feature,
                                                 std::string_view value_feature,
                                                 DataFrame::GroupOp op) const {
  return _ops::call(
      *this,
      _ops::PlotDispatch::HeatmapPivot,
      _ops::PlotCall{{case_feature, feature_feature, value_feature}, std::nullopt, std::nullopt, 20, op});
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(std::string_view case_feature,
                                                      std::string_view feature_feature,
                                                      std::string_view value_feature,
                                                      DataFrame::GroupOp op) const {
  auto pivoted = frame_.pivot_table(case_feature, feature_feature, value_feature, op, true, false);
  const auto& pivot_features = pivoted.feature_names();
  PLUSS_CHECK(!pivot_features.empty(),
              "DataFrame::plot().heatmap: pivot produced no columns for feature '" + std::string(feature_feature) + "'");

  std::vector<std::vector<double>> values(static_cast<std::size_t>(pivoted.nrows()),
                                          std::vector<double>(pivot_features.size(), std::numeric_limits<double>::quiet_NaN()));
  for (std::size_t c = 0; c < pivot_features.size(); ++c) {
    auto column_values = thread_to_plot_numeric_values(pivoted.features(pivot_features[c]));
    require_plot_size(column_values, pivoted.nrows(), pivot_features[c]);
    for (std::size_t r = 0; r < column_values.size(); ++r) {
      values[r][c] = column_values[r];
    }
  }

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false);
  auto& trace = add_heatmap(pane, values, std::string(value_feature));
  configure_heatmap(pane, trace, options_.heatmap_annotate_levels, options_.heatmap_scale_guide);
  pane.x_ticks(positional_plot_x(static_cast<std::int64_t>(pivot_features.size())),
               std::vector<std::string>(pivot_features.begin(), pivot_features.end()));
  pane.y_ticks(positional_plot_x(pivoted.nrows()),
               labels_for_cases(pivoted, "DataFrame::plot().heatmap"));
  pane.x_label(std::string(feature_feature)).y_label(std::string(case_feature));
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::corr_heatmap(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::CorrHeatmap, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::impl_corr_heatmap(std::initializer_list<std::string_view> features) const {
  const auto names = correlation_feature_names(frame_, features);
  auto fig =
      corr_heatmap_from_names(frame_, names, options_.corr_heatmap_annotate_levels, options_.heatmap_scale_guide);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::scatter_matrix(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::ScatterMatrix, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::impl_scatter_matrix(std::initializer_list<std::string_view> features) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().scatter_matrix");
  auto fig = scatter_matrix_from_names(frame_,
                                       names,
                                       options_.scatter_matrix_triangle,
                                       options_.scatter_matrix_monochrome,
                                       options_.scatter_matrix_auto_tick_decimals);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::auto_plot(std::initializer_list<std::string_view> features) const {
  return _ops::call(*this, _ops::PlotDispatch::AutoPlot, _ops::PlotCall{features});
}

pluss::pl::Studio DataFramePlotAccessor::impl_auto_plot(std::initializer_list<std::string_view> features) const {
  const bool explicit_features = features.size() > 0;
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().auto_plot");
  if (names.size() == 1) {
    if (thread_prefers_labeled_axis(frame_.cases()) && frame_.nrows() <= 16) {
      return bar(names.front());
    }
    return line(names.front());
  }
  if (!explicit_features && names.size() == 2) {
    const auto hue_names = hue_feature_names(frame_);
    if (!hue_names.empty()) {
      return scatter(names[0], names[1], hue_names.front());
    }
  }
  if (names.size() <= 3 && thread_prefers_labeled_axis(frame_.cases()) && all_nonnegative_columns(frame_, names)) {
    if (frame_.nrows() <= 12) {
      if (names.size() == 2) {
        return bar_stacked({names[0], names[1]});
      }
      return bar_stacked({names[0], names[1], names[2]});
    }
    if (names.size() == 2) {
      return area_stacked({names[0], names[1]});
    }
    return area_stacked({names[0], names[1], names[2]});
  }
  if (names.size() <= 4 && prefer_index_plot(frame_)) {
    switch (names.size()) {
      case 2:
        return line({names[0], names[1]});
      case 3:
        return line({names[0], names[1], names[2]});
      case 4:
        return line({names[0], names[1], names[2], names[3]});
      default:
        break;
    }
  }
  if (names.size() <= 5 && frame_.nrows() <= 256) {
    return scatter_matrix_from_names(frame_,
                                     names,
                                     options_.scatter_matrix_triangle,
                                     options_.scatter_matrix_monochrome,
                                     options_.scatter_matrix_auto_tick_decimals);
  }
  return corr_heatmap_from_names(frame_,
                                 names,
                                 options_.corr_heatmap_annotate_levels,
                                 options_.heatmap_scale_guide);
}

DataFramePlotAccessor DataFrame::plot() const {
  return DataFramePlotAccessor(*this);
}

DataFramePlotAccessor DataFrame::plot(const DataFramePlotOptions& options) const {
  return DataFramePlotAccessor(*this, options);
}

} // namespace pluss::table
