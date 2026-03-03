#include "table/plot/DataFramePlot.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include "CodeM/core/Errors.h"

namespace pluss::table {

namespace {

constexpr std::string_view kDefaultPlotTheme = "paper";
constexpr std::string_view kDefaultPlotFont = "IBM Plex Sans";

pluss::plotism::Style dataframe_series_style(std::size_t index,
                                             std::string marker = "none",
                                             double alpha = 1.0,
                                             double line_width = 1.5) {
  static const std::vector<std::string> palette{
      "#0f766e", "#c2410c", "#1d4ed8", "#b91c1c", "#4d7c0f",
      "#0369a1", "#b45309", "#0f172a", "#be123c", "#57534e"};
  pluss::plotism::Style style;
  style.color = palette[index % palette.size()];
  style.marker = std::move(marker);
  style.alpha = alpha;
  style.line_width = line_width;
  return style;
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
        std::string v(s);
        char* end = nullptr;
        const double parsed = std::strtod(v.c_str(), &end);
        const bool valid = (end != nullptr) && (end != v.c_str()) && (*end == '\0');
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

pluss::plotism::Figure make_dataframe_figure(bool show_legend, std::string legend_title = {}) {
  pluss::plotism::Figure fig;
  fig.theme(std::string(kDefaultPlotTheme)).font_family(std::string(kDefaultPlotFont));
  fig.legend(pluss::plotism::Legend{
      .visible = show_legend,
      .location = show_legend ? pluss::plotism::LegendLocation::OutsideRight : pluss::plotism::LegendLocation::Best,
      .title = std::move(legend_title),
  });
  return fig;
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

pluss::plotism::Figure corr_heatmap_from_names(const DataFrame& frame, const std::vector<std::string>& names) {
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

  auto fig = make_dataframe_figure(false);
  fig.add_heatmap(matrix, "corr", names, names);
  fig.x_label("feature").y_label("feature");
  fig.colorbar(pluss::plotism::Colorbar{
      .visible = true,
      .location = pluss::plotism::ColorbarLocation::Right,
      .title = "corr",
  });
  return fig;
}

pluss::plotism::Figure scatter_matrix_from_names(const DataFrame& frame, const std::vector<std::string>& names) {
  PLUSS_CHECK(names.size() >= 2, "DataFrame::plot().scatter_matrix: need at least two numeric features");

  std::vector<std::vector<double>> columns;
  columns.reserve(names.size());
  for (const auto& name : names) {
    auto values = thread_to_plot_numeric_values(frame.features(name));
    require_plot_size(values, frame.nrows(), name);
    columns.push_back(std::move(values));
  }

  const auto dims = static_cast<std::int32_t>(names.size());
  auto fig = make_dataframe_figure(false);
  for (std::int32_t row = 0; row < dims; ++row) {
    for (std::int32_t col = 0; col < dims; ++col) {
      fig.subplot(dims, dims, row * dims + col);
      if (row == col) {
        fig.add_hist(columns[static_cast<std::size_t>(col)], 12, names[static_cast<std::size_t>(col)]);
        fig.title(names[static_cast<std::size_t>(col)]);
      } else {
        fig.add_scatter(columns[static_cast<std::size_t>(col)],
                        columns[static_cast<std::size_t>(row)],
                        names[static_cast<std::size_t>(row)] + " vs " + names[static_cast<std::size_t>(col)]);
      }
      fig.x_label(row + 1 == dims ? names[static_cast<std::size_t>(col)] : "");
      fig.y_label(col == 0 ? names[static_cast<std::size_t>(row)] : "");
    }
  }
  return fig;
}

pluss::plotism::Figure box_from_names(const DataFrame& frame, const std::vector<std::string>& names) {
  PLUSS_CHECK(!names.empty(), "DataFrame::plot().box: at least one numeric feature is required");

  auto fig = make_dataframe_figure(false);
  std::vector<double> centers;
  std::vector<double> hidden_y;
  std::vector<std::string> labels;
  centers.reserve(names.size());
  hidden_y.reserve(names.size());
  labels.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    centers.push_back(static_cast<double>(i));
    hidden_y.push_back(std::numeric_limits<double>::quiet_NaN());
    labels.push_back(names[i]);
  }
  auto hidden_style = dataframe_series_style(0, "none", 0.0, 0.0);
  fig.add_line(centers, hidden_y, "", labels, hidden_style);

  constexpr double half_box_width = 0.28;
  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = thread_to_plot_numeric_values(frame.features(names[i]));
    require_plot_size(values, frame.nrows(), names[i]);
    const auto summary = summarize_boxplot(values);
    const double center = static_cast<double>(i);

    auto style = dataframe_series_style(i, "circle", 1.0, 1.4);
    fig.add_band({center - half_box_width, center + half_box_width},
                 {summary.q1, summary.q1},
                 {summary.q3, summary.q3},
                 names[i],
                 style);
    fig.add_line({center - half_box_width, center + half_box_width},
                 {summary.median, summary.median},
                 "",
                 {},
                 dataframe_series_style(i, "none", 1.0, 2.0));
    fig.add_line({center, center},
                 {summary.whisker_low, summary.q1},
                 "",
                 {},
                 dataframe_series_style(i, "none", 0.85, 1.2));
    fig.add_line({center, center},
                 {summary.q3, summary.whisker_high},
                 "",
                 {},
                 dataframe_series_style(i, "none", 0.85, 1.2));
    fig.add_line({center - 0.12, center + 0.12},
                 {summary.whisker_low, summary.whisker_low},
                 "",
                 {},
                 dataframe_series_style(i, "none", 0.85, 1.2));
    fig.add_line({center - 0.12, center + 0.12},
                 {summary.whisker_high, summary.whisker_high},
                 "",
                 {},
                 dataframe_series_style(i, "none", 0.85, 1.2));
    if (!summary.outliers.empty()) {
      std::vector<double> outlier_x(summary.outliers.size(), center);
      auto outlier_style = dataframe_series_style(i, "circle", 0.8, 1.0);
      outlier_style.marker_size = 2.4;
      fig.add_scatter(outlier_x, summary.outliers, "", {}, outlier_style);
    }
  }

  fig.x_label("feature").y_label("value");
  return fig;
}

template <typename AddTraceFn>
pluss::plotism::Figure make_xy_figure(const DataFrame& frame,
                                      std::initializer_list<std::string_view> y_features,
                                      std::optional<std::string_view> x_feature,
                                      std::string_view where,
                                      AddTraceFn&& add_trace) {
  PLUSS_CHECK(y_features.size() > 0, std::string(where) + ": at least one y feature is required");

  const auto axis = resolve_plot_axis(frame, x_feature, where);
  auto fig = make_dataframe_figure(y_features.size() > 1, y_features.size() > 1 ? "feature" : "");
  fig.x_label(axis.label);
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame.features(y_name));
    require_plot_size(y_values, frame.nrows(), y_name);
    add_trace(fig, axis, y_values, y_name);
  }
  fig.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));
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

} // namespace

DataFramePlotAccessor::DataFramePlotAccessor(DataFrame frame) : frame_(std::move(frame)) {}

pluss::plotism::Figure DataFramePlotAccessor::line(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return line({y_feature}, x_feature);
}

pluss::plotism::Figure DataFramePlotAccessor::line(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return make_xy_figure(frame_, y_features, x_feature, "DataFrame::plot().line",
                        [](pluss::plotism::Figure& fig,
                           const ResolvedPlotAxis& axis,
                           const std::vector<double>& y_values,
                           std::string_view name) {
                          fig.add_line(axis.values, y_values, std::string(name), axis.labels);
                        });
}

pluss::plotism::Figure DataFramePlotAccessor::step(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return step({y_feature}, x_feature);
}

pluss::plotism::Figure DataFramePlotAccessor::step(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return make_xy_figure(frame_, y_features, x_feature, "DataFrame::plot().step",
                        [](pluss::plotism::Figure& fig,
                           const ResolvedPlotAxis& axis,
                           const std::vector<double>& y_values,
                           std::string_view name) {
                          fig.add_step(axis.values, y_values, std::string(name), axis.labels);
                        });
}

pluss::plotism::Figure DataFramePlotAccessor::area(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return area({y_feature}, x_feature);
}

pluss::plotism::Figure DataFramePlotAccessor::area(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return make_xy_figure(frame_, y_features, x_feature, "DataFrame::plot().area",
                        [](pluss::plotism::Figure& fig,
                           const ResolvedPlotAxis& axis,
                           const std::vector<double>& y_values,
                           std::string_view name) {
                          fig.add_area(axis.values, y_values, std::string(name));
                        });
}

pluss::plotism::Figure DataFramePlotAccessor::area_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().area_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().area_stacked");
  auto fig = make_dataframe_figure(y_features.size() > 1, y_features.size() > 1 ? "feature" : "");
  fig.x_label(axis.label).y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  std::vector<double> baselines(static_cast<std::size_t>(frame_.nrows()), 0.0);
  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(heights, frame_.nrows(), y_name);
    std::vector<double> tops(heights.size(), 0.0);
    for (std::size_t i = 0; i < heights.size(); ++i) {
      tops[i] = baselines[i] + heights[i];
    }
    auto style = dataframe_series_style(feature_index, "none", 1.0, 1.6);
    fig.add_band(axis.values, baselines, tops, std::string(y_name), style);
    fig.add_line(axis.values, tops, "", axis.labels, style);
    baselines = std::move(tops);
    ++feature_index;
  }
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::scatter(std::string_view x_feature,
                                                      std::string_view y_feature,
                                                      std::optional<std::string_view> hue_feature) const {
  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().scatter");
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, frame_.nrows(), y_feature);

  auto fig = make_dataframe_figure(hue_feature.has_value(), hue_feature.has_value() ? std::string(*hue_feature) : "");
  if (!hue_feature.has_value()) {
    fig.add_scatter(axis.values, y_values, std::string(y_feature), axis.labels);
    fig.x_label(axis.label).y_label(std::string(y_feature));
    return fig;
  }

  const auto hue_thread = frame_.features(*hue_feature);
  PLUSS_CHECK(thread_supports_hue(hue_thread),
              "DataFrame::plot().scatter: hue feature '" + std::string(*hue_feature) + "' must be note-like or time-like");
  const auto hue_labels = thread_to_plot_labels(hue_thread);
  require_plot_size(hue_labels, frame_.nrows(), *hue_feature, "DataFrame::plot().scatter");

  std::vector<std::string> groups;
  for (const auto& label : hue_labels) {
    if (std::find(groups.begin(), groups.end(), label) == groups.end()) {
      groups.push_back(label);
    }
  }

  for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
    std::vector<double> grouped_x;
    std::vector<double> grouped_y;
    std::vector<std::string> grouped_labels;
    for (std::size_t i = 0; i < hue_labels.size(); ++i) {
      if (hue_labels[i] != groups[group_index]) {
        continue;
      }
      grouped_x.push_back(axis.values[i]);
      grouped_y.push_back(y_values[i]);
      if (!axis.labels.empty()) {
        grouped_labels.push_back(axis.labels[i]);
      }
    }
    auto style = dataframe_series_style(group_index, "circle", 0.92, 1.2);
    style.marker_size = 3.0;
    fig.add_scatter(grouped_x, grouped_y, groups[group_index], std::move(grouped_labels), style);
  }
  fig.x_label(axis.label).y_label(std::string(y_feature));
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return bar({y_feature}, x_feature);
}

pluss::plotism::Figure DataFramePlotAccessor::bar(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().bar");
  auto fig = make_dataframe_figure(y_features.size() > 1, y_features.size() > 1 ? "feature" : "");
  fig.x_label(axis.label).y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  const std::size_t group_count = y_features.size();
  std::size_t group_index = 0;
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(y_values, frame_.nrows(), y_name);
    auto [centers, widths, labels] = grouped_bar_layout(axis, group_index, group_count);
    fig.add_bar(centers, y_values, std::string(y_name), std::move(labels), std::move(widths));
    ++group_index;
  }
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::bar_stacked(
    std::initializer_list<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().bar_stacked");
  auto fig = make_dataframe_figure(y_features.size() > 1, y_features.size() > 1 ? "feature" : "");
  fig.x_label(axis.label).y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  std::vector<double> baselines(static_cast<std::size_t>(frame_.nrows()), 0.0);
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(heights, frame_.nrows(), y_name);
    std::vector<double> tops(heights.size(), 0.0);
    for (std::size_t i = 0; i < heights.size(); ++i) {
      tops[i] = baselines[i] + heights[i];
    }
    fig.add_bar(axis.values, tops, std::string(y_name), axis.labels, {}, baselines);
    baselines = std::move(tops);
  }
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::box(std::initializer_list<std::string_view> features) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().box");
  return box_from_names(frame_, names);
}

pluss::plotism::Figure DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins) const {
  auto fig = make_dataframe_figure(false);
  fig.add_hist(thread_to_plot_numeric_values(frame_.features(feature)), bins, std::string(feature));
  fig.x_label(std::string(feature)).y_label("count");
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::heatmap(std::initializer_list<std::string_view> features) const {
  PLUSS_CHECK(features.size() > 0, "DataFrame::plot().heatmap: at least one feature is required");

  auto fig = make_dataframe_figure(false);
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

  fig.add_heatmap(values, "value", labels_for_cases(frame_, "DataFrame::plot().heatmap"), std::move(y_labels));
  fig.x_label("case").y_label("feature");
  fig.colorbar(pluss::plotism::Colorbar{.visible = true, .location = pluss::plotism::ColorbarLocation::Right, .title = "value"});
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::heatmap(std::string_view case_feature,
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

  auto fig = make_dataframe_figure(false);
  fig.add_heatmap(values,
                  std::string(value_feature),
                  std::vector<std::string>(pivot_features.begin(), pivot_features.end()),
                  labels_for_cases(pivoted, "DataFrame::plot().heatmap"));
  fig.x_label(std::string(feature_feature)).y_label(std::string(case_feature));
  fig.colorbar(pluss::plotism::Colorbar{
      .visible = true,
      .location = pluss::plotism::ColorbarLocation::Right,
      .title = std::string(value_feature),
  });
  return fig;
}

pluss::plotism::Figure DataFramePlotAccessor::corr_heatmap(std::initializer_list<std::string_view> features) const {
  const auto names = correlation_feature_names(frame_, features);
  return corr_heatmap_from_names(frame_, names);
}

pluss::plotism::Figure DataFramePlotAccessor::scatter_matrix(std::initializer_list<std::string_view> features) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().scatter_matrix");
  return scatter_matrix_from_names(frame_, names);
}

pluss::plotism::Figure DataFramePlotAccessor::auto_plot(std::initializer_list<std::string_view> features) const {
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
    return scatter_matrix_from_names(frame_, names);
  }
  return corr_heatmap_from_names(frame_, names);
}

DataFramePlotAccessor DataFrame::plot() const {
  return DataFramePlotAccessor(*this);
}

} // namespace pluss::table
