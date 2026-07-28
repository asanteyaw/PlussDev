#include "table/plot/DataFramePlot.h"
#include "table/plot/detail/Plot3DConfig.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
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

void setup_pane(Pane& pane, bool show_legend, const DataFramePlotOptions* options = nullptr) {
  pane.grid(pluss::pl::GridStyle::Major);
  pane.legend(show_legend ? pluss::pl::KeyPlace::Auto : pluss::pl::KeyPlace::None);
  if (options != nullptr) {
    if (options->x_label_rotation_deg.has_value()) {
      pane.x_label_rotation(*options->x_label_rotation_deg);
    }
    if (options->y_label_rotation_deg.has_value()) {
      pane.y_label_rotation(*options->y_label_rotation_deg);
    }
    if (options->x_tick_format.has_value()) {
      pane.x_tick_format(*options->x_tick_format);
    }
    if (options->y_tick_format.has_value()) {
      pane.y_tick_format(*options->y_tick_format);
    }
    if (options->x_tick_label_gap_points.has_value()) {
      pane.x_tick_label_gap(*options->x_tick_label_gap_points);
    }
    if (options->y_tick_label_gap_points.has_value()) {
      pane.y_tick_label_gap(*options->y_tick_label_gap_points);
    }
    if (options->x_label_gap_points.has_value()) {
      pane.x_label_gap(*options->x_label_gap_points);
    }
    if (options->y_label_gap_points.has_value()) {
      pane.y_label_gap(*options->y_label_gap_points);
    }
  }
}

std::optional<double> resolved_x_tick_rotation(const DataFramePlotOptions& options) {
  if (options.x_tick_rotation_deg.has_value()) {
    return options.x_tick_rotation_deg;
  }
  if (options.labeled_tick_rotation_deg.has_value()) {
    return options.labeled_tick_rotation_deg;
  }
  return std::nullopt;
}

void apply_x_tick_rotation(Pane& pane,
                           const DataFramePlotOptions& options,
                           bool default_if_crowded = false) {
  if (const auto rot = resolved_x_tick_rotation(options); rot.has_value()) {
    pane.x_tick_rotation(*rot);
  } else if (default_if_crowded) {
    pane.x_tick_rotation(45.0);
  }
}

void apply_y_tick_rotation(Pane& pane,
                           const DataFramePlotOptions& options,
                           std::optional<double> default_rotation = std::nullopt) {
  if (options.y_tick_rotation_deg.has_value()) {
    pane.y_tick_rotation(*options.y_tick_rotation_deg);
    return;
  }
  if (default_rotation.has_value()) {
    pane.y_tick_rotation(*default_rotation);
  }
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

pluss::pl::ViolinOptions to_core_violin_options(const ViolinPlotOptions& options) {
  pluss::pl::ViolinOptions out;
  out.width = options.width;
  out.bw_adjust = options.bw_adjust;
  out.cut = options.cut;
  out.scale = options.scale;
  out.inner = options.inner;
  out.side = options.side;
  return out;
}

std::vector<double> positional_plot_x(std::int64_t nrows) {
  std::vector<double> out(static_cast<std::size_t>(nrows), 0.0);
  std::iota(out.begin(), out.end(), 0.0);
  return out;
}

pluss::Series thread_to_plot_numeric_series(const Thread& t) {
  Thread numeric_thread = t;
  switch (t.kind()) {
    case ThreadKind::Series: {
      break;
    }
    case ThreadKind::Timeline: {
      numeric_thread = Thread(t.as_timeline().epoch_nanos(), detail::validity_mask(t));
      break;
    }
    case ThreadKind::TimeIndex: {
      numeric_thread = Thread(t.as_timeindex().timeline().epoch_nanos(), detail::validity_mask(t));
      break;
    }
    case ThreadKind::Note: {
      numeric_thread = t.cast({.series_dtype = pluss::codem::ScalarType::Float64}).series();
      break;
    }
    default:
      PLUSS_FAIL("DataFrame::plot: unsupported Thread kind for numeric plotting");
  }

  auto values = numeric_thread.as_series().to(pluss::codem::ScalarType::Float64);
  if (detail::has_validity_mask(numeric_thread)) {
    const auto nan_fill = pluss::Series::full(
        {values.numel()},
        std::numeric_limits<double>::quiet_NaN(),
        pluss::codem::ScalarType::Float64);
    values = values.where(detail::validity_mask(numeric_thread)->logical_not(), nan_fill);
  }
  return values.reshape({values.numel()});
}

std::vector<double> thread_to_plot_numeric_values(const Thread& t) {
  auto values = thread_to_plot_numeric_series(t);
  return values.to_vec<double>();
}

bool series_all_zero_i64(const pluss::Series& s) {
  return !s.to(pluss::codem::ScalarType::Int64).ne(0).any().item<bool>();
}

bool series_all_equal_i64(const pluss::Series& s, std::int64_t target) {
  return !s.to(pluss::codem::ScalarType::Int64).ne(target).any().item<bool>();
}

enum class TemporalLabelKind : std::uint8_t {
  Date,
  Time,
  DateTime,
};

TemporalLabelKind infer_temporal_label_kind(const pluss::Timeline& timeline) {
  const auto has_subsecond = !series_all_zero_i64(timeline.nanosecond());
  const auto has_clock_time = has_subsecond || !series_all_zero_i64(timeline.second())
                              || !series_all_zero_i64(timeline.minute())
                              || !series_all_zero_i64(timeline.hour());
  if (!has_clock_time) {
    return TemporalLabelKind::Date;
  }

  // Time-only values in CSV/Timeline APIs are anchored to epoch day 1970-01-01.
  const auto epoch_day_anchor = series_all_equal_i64(timeline.year(), 1970)
                                && series_all_equal_i64(timeline.month(), 1)
                                && series_all_equal_i64(timeline.day(), 1);
  if (epoch_day_anchor) {
    return TemporalLabelKind::Time;
  }
  return TemporalLabelKind::DateTime;
}

std::vector<std::string> temporal_thread_to_plot_labels(const Thread& t, DateTimeLabelView datetime_label_view) {
  const auto timeline = (t.kind() == ThreadKind::Timeline) ? t.as_timeline() : t.as_timeindex().timeline();
  const auto has_subsecond = !series_all_zero_i64(timeline.nanosecond());
  const auto rendered = [&]() -> pluss::Note {
    switch (infer_temporal_label_kind(timeline)) {
      case TemporalLabelKind::Date:
        return timeline.to_note_date();
      case TemporalLabelKind::Time:
        return has_subsecond ? timeline.strftime("%H:%M:%S.%f") : timeline.to_note_time();
      case TemporalLabelKind::DateTime:
        switch (datetime_label_view) {
          case DateTimeLabelView::Date:
            return timeline.to_note_date();
          case DateTimeLabelView::Time:
            return has_subsecond ? timeline.strftime("%H:%M:%S.%f") : timeline.to_note_time();
          case DateTimeLabelView::Full:
          default:
            return has_subsecond ? timeline.to_note_datetime_nano() : timeline.to_note_datetime();
        }
    }
    return timeline.to_note_datetime();
  }();

  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(rendered.numel()));
  rendered.for_each_flat([&](std::string_view s) { out.emplace_back(s); });
  return out;
}

std::vector<std::string> thread_to_plot_labels(const Thread& t, const DataFramePlotOptions& options) {
  if (t.kind() == ThreadKind::Timeline || t.kind() == ThreadKind::TimeIndex) {
    return temporal_thread_to_plot_labels(t, options.datetime_label_view);
  }
  const auto note_like = (t.kind() == ThreadKind::Note) ? t : t.cast().note();
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

void require_plot_size(const pluss::Series& values, std::int64_t expected, std::string_view feature_name) {
  PLUSS_CHECK(values.numel() == expected,
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
                                   std::string_view where,
                                   const DataFramePlotOptions& options) {
  const auto nrows = frame.ncases();
  const auto axis_name = x_feature.has_value() ? std::string(*x_feature) : std::string("case");
  const auto axis_thread = x_feature.has_value() ? frame.features(*x_feature) : frame.label().values();

  ResolvedPlotAxis axis;
  axis.label = axis_name;
  if (thread_prefers_labeled_axis(axis_thread)) {
    axis.values = positional_plot_x(nrows);
    axis.labels = thread_to_plot_labels(axis_thread, options);
    require_plot_size(axis.labels, nrows, axis_name, where);
    return axis;
  }

  axis.values = thread_to_plot_numeric_values(axis_thread);
  require_plot_size(axis.values, nrows, axis_name);
  return axis;
}

std::pair<std::vector<double>, std::vector<std::string>> sampled_labeled_ticks(
    const ResolvedPlotAxis& axis,
    std::int64_t max_ticks) {
  const auto total = std::min(axis.values.size(), axis.labels.size());
  if (total == 0) {
    return {};
  }
  const std::size_t capped = static_cast<std::size_t>(std::max<std::int64_t>(2, max_ticks));
  if (total <= capped) {
    return {axis.values, axis.labels};
  }

  std::vector<double> sampled_values;
  std::vector<std::string> sampled_labels;
  sampled_values.reserve(capped);
  sampled_labels.reserve(capped);

  std::size_t prev = std::numeric_limits<std::size_t>::max();
  for (const auto i : std::views::iota(std::size_t{0}, capped)) {
    const auto idx = static_cast<std::size_t>(
        std::llround(static_cast<double>(i) * static_cast<double>(total - 1) / static_cast<double>(capped - 1)));
    if (idx == prev) {
      continue;
    }
    sampled_values.push_back(axis.values[idx]);
    sampled_labels.push_back(axis.labels[idx]);
    prev = idx;
  }
  return {std::move(sampled_values), std::move(sampled_labels)};
}

bool looks_like_ymd_prefix(std::string_view text) {
  return text.size() >= 10
         && std::isdigit(static_cast<unsigned char>(text[0]))
         && std::isdigit(static_cast<unsigned char>(text[1]))
         && std::isdigit(static_cast<unsigned char>(text[2]))
         && std::isdigit(static_cast<unsigned char>(text[3]))
         && text[4] == '-'
         && std::isdigit(static_cast<unsigned char>(text[5]))
         && std::isdigit(static_cast<unsigned char>(text[6]))
         && text[7] == '-'
         && std::isdigit(static_cast<unsigned char>(text[8]))
         && std::isdigit(static_cast<unsigned char>(text[9]));
}

std::string simplify_midnight_datetime_label(std::string_view label) {
  if (!looks_like_ymd_prefix(label)) {
    return std::string(label);
  }
  if (label.find("T00:00:00") == 10 || label.find(" 00:00:00") == 10) {
    return std::string(label.substr(0, 10));
  }
  return std::string(label);
}

template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_reference_t<const Range>, std::string_view>
[[nodiscard]] std::vector<std::string_view> label_views(const Range& labels) {
  std::vector<std::string_view> out;
  if constexpr (requires { labels.size(); }) {
    out.reserve(static_cast<std::size_t>(labels.size()));
  }
  for (const auto& label : labels) {
    out.emplace_back(std::string_view(label));
  }
  return out;
}

void apply_axis(Pane& pane, const ResolvedPlotAxis& axis, const DataFramePlotOptions& options) {
  pane.x_label(axis.label);
  if (!axis.labels.empty()) {
    const auto [tick_values, tick_labels] = sampled_labeled_ticks(axis, options.max_labeled_ticks);
    std::vector<std::string> normalized_labels;
    normalized_labels.reserve(tick_labels.size());
    std::transform(tick_labels.begin(), tick_labels.end(), std::back_inserter(normalized_labels),
                   [](const std::string& label) { return simplify_midnight_datetime_label(label); });
    const auto tick_label_views = label_views(normalized_labels);
    pane.x_ticks(tick_values, pluss::msharp::RangeView::Of<std::string_view>(tick_label_views));
    apply_x_tick_rotation(pane, options, axis.labels.size() > tick_labels.size());
  }
}

std::vector<std::string> labels_for_cases(const DataFrame& frame,
                                          std::string_view where,
                                          const DataFramePlotOptions& options) {
  auto labels = thread_to_plot_labels(frame.label().values(), options);
  require_plot_size(labels, frame.ncases(), "case", where);
  return labels;
}

bool thread_supports_correlation(const Thread& thread) {
  return thread.kind() == ThreadKind::Series;
}

bool thread_supports_hue(const Thread& thread) {
  return thread.kind() == ThreadKind::Note || thread.kind() == ThreadKind::Timeline || thread.kind() == ThreadKind::TimeIndex;
}

std::vector<std::string> numeric_feature_names(const DataFrame& frame,
                                               pluss::msharp::RangeView::Of<std::string_view> requested,
                                               std::string_view where);

std::vector<std::string> hue_feature_names(const DataFrame& frame) {
  const auto features = frame.features();
  std::vector<std::string> names;
  names.reserve(features.size());
  std::copy_if(
      features.begin(),
      features.end(),
      std::back_inserter(names),
      [&](const std::string& feature) { return thread_supports_hue(frame.features(feature)); });
  return names;
}

std::vector<std::string> correlation_feature_names(const DataFrame& frame,
                                                   pluss::msharp::RangeView::Of<std::string_view> requested) {
  return numeric_feature_names(frame, requested, "DataFrame::plot().corr_heatmap");
}

std::vector<std::string> numeric_feature_names(const DataFrame& frame,
                                               pluss::msharp::RangeView::Of<std::string_view> requested,
                                               std::string_view where) {
  std::vector<std::string> names;
  if (requested.size() > 0) {
    names.reserve(requested.size());
    std::transform(
        requested.begin(),
        requested.end(),
        std::back_inserter(names),
        [&](std::string_view feature) {
          const auto thread = frame.features(feature);
          PLUSS_CHECK(
              thread_supports_correlation(thread),
              std::string(where) + ": feature '" + std::string(feature) + "' is not a numeric Series");
          return std::string(feature);
        });
    return names;
  }

  const auto features = frame.features();
  names.reserve(features.size());
  std::copy_if(
      features.begin(),
      features.end(),
      std::back_inserter(names),
      [&](const std::string& feature) { return thread_supports_correlation(frame.features(feature)); });
  PLUSS_CHECK(!names.empty(), std::string(where) + ": no numeric Series features available");
  return names;
}

std::vector<pluss::Series> numeric_series_columns(const DataFrame& frame,
                                                  const std::vector<std::string>& names,
                                                  std::string_view where) {
  std::vector<pluss::Series> columns;
  columns.reserve(names.size());
  std::transform(
      names.begin(),
      names.end(),
      std::back_inserter(columns),
      [&](const std::string& name) {
        const auto thread = frame.features(name);
        PLUSS_CHECK(
            thread.kind() == ThreadKind::Series,
            std::string(where) + ": feature '" + name + "' is not a numeric Series");
        auto values = thread.as_series().to(pluss::codem::ScalarType::Float64).reshape({thread.as_series().numel()});
        if (detail::has_validity_mask(thread)) {
          const auto nan_fill = pluss::Series::full(
              {values.numel()},
              std::numeric_limits<double>::quiet_NaN(),
              pluss::codem::ScalarType::Float64);
          values = values.where(detail::validity_mask(thread)->logical_not(), nan_fill);
        }
        return values;
      });
  PLUSS_CHECK(!columns.empty(), std::string(where) + ": no numeric columns available");
  return columns;
}

pluss::Note thread_to_plot_note(const Thread& t, std::string_view where, std::string_view feature_name) {
  const auto note_like = (t.kind() == ThreadKind::Note) ? t : t.cast().note();
  PLUSS_CHECK(note_like.kind() == ThreadKind::Note,
              std::string(where) + ": feature '" + std::string(feature_name) + "' cannot be converted to Note");
  return note_like.as_note();
}

bool all_nonnegative_columns(const DataFrame& frame, const std::vector<std::string>& names) {
  return std::all_of(
      names.begin(),
      names.end(),
      [&](const std::string& name) {
        auto values = thread_to_plot_numeric_series(frame.features(name));
        values = values.nan_to_num(0.0, 0.0, 0.0);
        return !values.lt(0.0).any().item<bool>();
      });
}

bool prefer_index_plot(const DataFrame& frame) {
  return thread_prefers_labeled_axis(frame.label().values()) || frame.ncases() > 24;
}

pluss::Series columns_to_matrix_series(const std::vector<pluss::Series>& columns,
                                       std::int64_t rows,
                                       bool nonfinite_to_zero) {
  PLUSS_CHECK(!columns.empty(), "DataFrame::plot: expected at least one column");
  std::vector<pluss::Series> prepared;
  prepared.reserve(columns.size());
  std::transform(columns.begin(), columns.end(), std::back_inserter(prepared), [&](const pluss::Series& col) {
    auto flat = col.to(pluss::codem::ScalarType::Float64).reshape({rows});
    return nonfinite_to_zero ? flat.nan_to_num(0.0, 0.0, 0.0) : flat;
  });
  auto linear = prepared.front();
  if (prepared.size() > 1) {
    std::vector<pluss::Series> tail;
    tail.reserve(prepared.size() - 1);
    std::copy(prepared.begin() + 1, prepared.end(), std::back_inserter(tail));
    linear = linear.unite(tail, 0);
  }
  return linear.reshape({static_cast<std::int64_t>(prepared.size()), rows}).transpose(0, 1).contiguous();
}

flash::HeatmapOptions to_flash_options(const HeatmapPlotOptions& options) {
  flash::HeatmapOptions out;
  out.annotate_levels = options.annotate_levels;
  out.scale_guide = options.scale_guide;
  out.borderless = options.borderless;
  return out;
}

flash::HeatmapOptions to_flash_options(const CorrHeatmapPlotOptions& options) {
  flash::HeatmapOptions out;
  out.annotate_levels = options.annotate_levels;
  out.scale_guide = options.scale_guide;
  out.borderless = options.borderless;
  return out;
}

Studio corr_heatmap_from_names(const DataFrame& frame,
                               const std::vector<std::string>& names,
                               const CorrHeatmapPlotOptions& options,
                               const DataFramePlotOptions& plot_options) {
  const auto corr = frame.corr(false);
  const auto corr_names = corr.features();
  std::unordered_map<std::string, std::size_t> corr_index;
  corr_index.reserve(corr_names.size());
  for (std::size_t i = 0; i < corr_names.size(); ++i) {
    corr_index.emplace(corr_names[i], i);
  }

  std::vector<std::int64_t> row_order;
  row_order.reserve(names.size());
  std::transform(
      names.begin(),
      names.end(),
      std::back_inserter(row_order),
      [&](const std::string& name) {
        const auto it = corr_index.find(name);
        PLUSS_CHECK(it != corr_index.end(),
                    "DataFrame::plot().corr_heatmap: missing correlation feature '" + name + "'");
        return static_cast<std::int64_t>(it->second);
      });
  const auto row_index = pluss::make_series(row_order, {static_cast<std::int64_t>(row_order.size())});

  std::vector<pluss::Series> columns;
  columns.reserve(names.size());
  std::transform(
      names.begin(),
      names.end(),
      std::back_inserter(columns),
      [&](const std::string& name) {
        return corr.features(name)
            .as_series()
            .to(pluss::codem::ScalarType::Float64)
            .reshape({corr.ncases()})
            .take(row_index);
      });

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false, &plot_options);
  const auto axis_positions = positional_plot_x(static_cast<std::int64_t>(names.size()));
  auto z = columns_to_matrix_series(columns, static_cast<std::int64_t>(names.size()), false);
  auto& trace = flash::heatmap(pane, axis_positions, axis_positions, z, to_flash_options(options));
  trace.label("corr").shade(pluss::pl::ShadeMode::Smooth);

  const auto name_views = label_views(names);
  const auto name_range = pluss::msharp::RangeView::Of<std::string_view>(name_views);
  pane.x_ticks(axis_positions, name_range).y_ticks(axis_positions, name_range);
  apply_x_tick_rotation(pane, plot_options, false);
  apply_y_tick_rotation(pane, plot_options, options.borderless ? std::nullopt : std::optional<double>{30.0});
  pane.x_label("feature").y_label("feature");
  return fig;
}

Studio scatter_matrix_from_names(const DataFrame& frame,
                                 const std::vector<std::string>& names,
                                 const flash::PairPlotOptions& options,
                                 const DataFramePlotOptions& plot_options) {
  PLUSS_CHECK(names.size() >= 2, "DataFrame::plot().scatter_matrix: need at least two numeric features");

  std::vector<pluss::Series> columns;
  columns.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(columns), [&](const std::string& name) {
    return frame.features(name).as_series();
  });

  auto pair_options = options;
  if (pair_options.labels.empty()) {
    pair_options.labels = names;
  }
  if (!pair_options.x_tick_rotation_deg.has_value()) {
    pair_options.x_tick_rotation_deg = resolved_x_tick_rotation(plot_options);
  }
  if (!pair_options.y_tick_rotation_deg.has_value()) {
    pair_options.y_tick_rotation_deg = plot_options.y_tick_rotation_deg;
  }
  if (!pair_options.x_label_rotation_deg.has_value()) {
    pair_options.x_label_rotation_deg = plot_options.x_label_rotation_deg;
  }
  if (!pair_options.y_label_rotation_deg.has_value()) {
    pair_options.y_label_rotation_deg = plot_options.y_label_rotation_deg;
  }
  if (!pair_options.x_tick_format.has_value()) {
    pair_options.x_tick_format = plot_options.x_tick_format;
  }
  if (!pair_options.y_tick_format.has_value()) {
    pair_options.y_tick_format = plot_options.y_tick_format;
  }
  if (!pair_options.x_tick_label_gap_points.has_value()) {
    pair_options.x_tick_label_gap_points = plot_options.x_tick_label_gap_points;
  }
  if (!pair_options.y_tick_label_gap_points.has_value()) {
    pair_options.y_tick_label_gap_points = plot_options.y_tick_label_gap_points;
  }
  if (!pair_options.x_label_gap_points.has_value()) {
    pair_options.x_label_gap_points = plot_options.x_label_gap_points;
  }
  if (!pair_options.y_label_gap_points.has_value()) {
    pair_options.y_label_gap_points = plot_options.y_label_gap_points;
  }

  auto fig = make_studio();
  flash::pairplot(fig, columns, pair_options);
  return fig;
}

flash::PairPlotOptions resolve_scatter_matrix_pair_options(const ScatterMatrixPlotOptions& options) {
  auto pair_options = options.pair;
  if (options.corner && pair_options.triangle == ScatterMatrixTriangle::Full) {
    pair_options.triangle = ScatterMatrixTriangle::Lower;
  }
  return pair_options;
}

flash::ParallelCoordinatesOptions to_flash_options(const ParallelPlotOptions& options) {
  flash::ParallelCoordinatesOptions out;
  out.labels = options.labels;
  out.normalize = options.normalize;
  out.max_points = options.max_points;
  out.alpha = options.alpha;
  out.trace_width = options.trace_width;
  out.show_grid = options.show_grid;
  out.show_legend = options.show_legend;
  return out;
}

flash::AndrewsCurvesOptions to_flash_options(const AndrewsCurvesPlotOptions& options) {
  flash::AndrewsCurvesOptions out;
  out.labels = options.labels;
  out.normalize = options.normalize;
  out.points = options.points;
  out.max_curves = options.max_curves;
  out.alpha = options.alpha;
  out.trace_width = options.trace_width;
  out.show_grid = options.show_grid;
  out.show_legend = options.show_legend;
  return out;
}

flash::RadvizOptions to_flash_options(const RadvizPlotOptions& options) {
  flash::RadvizOptions out;
  out.labels = options.labels;
  out.normalize = options.normalize;
  out.max_points = options.max_points;
  out.marker_area = options.marker_area;
  out.trace_width = options.trace_width;
  out.show_grid = options.show_grid;
  out.show_anchors = options.show_anchors;
  out.show_legend = options.show_legend;
  return out;
}

flash::LagPlotOptions to_flash_options(const LagPlotOptions& options) {
  flash::LagPlotOptions out;
  out.lag = options.lag;
  out.max_points = options.max_points;
  out.marker_area = options.marker_area;
  out.trace_width = options.trace_width;
  return out;
}

flash::AutocorrelationPlotOptions to_flash_options(const ActPlotOptions& options) {
  flash::AutocorrelationPlotOptions out;
  out.max_lag = options.max_lag;
  out.show_confidence_band = options.show_confidence_band;
  out.confidence_level = options.confidence_level;
  out.trace_width = options.trace_width;
  return out;
}

flash::BootstrapStatistic to_flash_bootstrap_stat(BootstrapStatistic statistic) {
  switch (statistic) {
    case BootstrapStatistic::Median:
      return flash::BootstrapStatistic::Median;
    case BootstrapStatistic::Std:
      return flash::BootstrapStatistic::Std;
    case BootstrapStatistic::Mean:
    default:
      return flash::BootstrapStatistic::Mean;
  }
}

flash::BootstrapPlotOptions to_flash_options(const BootstrapPlotOptions& options) {
  flash::BootstrapPlotOptions out;
  out.samples = options.samples;
  out.sample_size = options.sample_size;
  out.seed = options.seed;
  out.bins = options.bins;
  out.statistic = to_flash_bootstrap_stat(options.statistic);
  out.trace_width = options.trace_width;
  return out;
}

flash::SummaryOptions to_flash_options(const BriefPlotOptions& options) {
  flash::SummaryOptions out;
  out.title = options.title;
  out.precision = options.precision;
  out.include_header = options.include_header;
  out.max_cases = options.max_cases;
  out.transpose = options.transpose;
  out.include_case_index = options.include_case_index;
  out.features_per_table = options.features_per_table;
  out.tables_per_row = options.tables_per_row;
  out.table_gap_spaces = options.table_gap_spaces;
  return out;
}

Studio box_from_names(const DataFrame& frame,
                      const std::vector<std::string>& names,
                      const BoxPlotOptions& options,
                      const DataFramePlotOptions& plot_options) {
  PLUSS_CHECK(!names.empty(), "DataFrame::plot().box: at least one numeric feature is required");

  std::vector<double> group_id;
  std::vector<double> merged_values;
  group_id.reserve(static_cast<std::size_t>(frame.ncases()) * names.size());
  merged_values.reserve(static_cast<std::size_t>(frame.ncases()) * names.size());

  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = thread_to_plot_numeric_values(frame.features(names[i]));
    require_plot_size(values, frame.ncases(), names[i]);
    std::for_each(values.begin(), values.end(), [&](double value) {
      if (std::isfinite(value)) {
        group_id.push_back(static_cast<double>(i));
        merged_values.push_back(value);
      }
    });
  }

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false, &plot_options);
  auto& trace = flash::boxplot(pane, group_id, merged_values);
  trace.label("distribution");
  style_trace(trace, 0, options.alpha, options.line_width);

  const auto centers = positional_plot_x(static_cast<std::int64_t>(names.size()));
  const auto name_views = label_views(names);
  pane.x_ticks(centers, pluss::msharp::RangeView::Of<std::string_view>(name_views));
  apply_x_tick_rotation(pane, plot_options, false);
  pane.x_label("feature").y_label("value");
  return fig;
}

Studio violin_from_names(const DataFrame& frame,
                         const std::vector<std::string>& names,
                         const ViolinPlotOptions& options,
                         const DataFramePlotOptions& plot_options) {
  PLUSS_CHECK(!names.empty(), "DataFrame::plot().violin: at least one numeric feature is required");

  std::vector<double> group_id;
  std::vector<double> merged_values;
  group_id.reserve(static_cast<std::size_t>(frame.ncases()) * names.size());
  merged_values.reserve(static_cast<std::size_t>(frame.ncases()) * names.size());

  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = thread_to_plot_numeric_values(frame.features(names[i]));
    require_plot_size(values, frame.ncases(), names[i]);
    std::for_each(values.begin(), values.end(), [&](double value) {
      if (std::isfinite(value)) {
        group_id.push_back(static_cast<double>(i));
        merged_values.push_back(value);
      }
    });
  }

  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, false, &plot_options);
  auto& trace = flash::violinplot(
      pane,
      group_id,
      merged_values,
      to_core_violin_options(options));
  trace.label("distribution");
  style_trace(trace, 0, options.alpha, options.line_width);

  const auto centers = positional_plot_x(static_cast<std::int64_t>(names.size()));
  const auto name_views = label_views(names);
  pane.x_ticks(centers, pluss::msharp::RangeView::Of<std::string_view>(name_views));
  apply_x_tick_rotation(pane, plot_options, false);
  pane.x_label("feature").y_label("value");
  return fig;
}

template <typename AddTraceFn>
Studio make_xy_figure(const DataFrame& frame,
                      pluss::msharp::RangeView::Of<std::string_view> y_features,
                      std::optional<std::string_view> x_feature,
                      std::string_view where,
                      AddTraceFn&& add_trace,
                      const DataFramePlotOptions& options,
                      std::optional<pluss::pl::LimitMethod> y_limit_method = std::nullopt) {
  PLUSS_CHECK(y_features.size() > 0, std::string(where) + ": at least one y feature is required");

  const auto axis = resolve_plot_axis(frame, x_feature, where, options);
  auto fig = make_studio();
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1, &options);
  apply_axis(pane, axis, options);

  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame.features(y_name));
    require_plot_size(y_values, frame.ncases(), y_name);
    add_trace(pane, axis, y_values, y_name, feature_index);
    ++feature_index;
  }
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));
  if (y_limit_method.has_value()) {
    pane.y_limit_method(*y_limit_method);
  }
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

  std::size_t index = 0;
  std::for_each(centers.begin(), centers.end(), [&](double& center) {
    const auto [slot_lo, slot_hi] = cell_bounds(axis.values, index);
    const double slot_width = std::max(1e-9, slot_hi - slot_lo);
    const double group_span = slot_width * 0.82;
    const double bar_width = group_span / static_cast<double>(group_count);
    center = slot_lo + 0.5 * (slot_width - group_span) + bar_width * (static_cast<double>(group_index) + 0.5);
    widths[index] = bar_width * 0.92;
    ++index;
  });
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

struct ScatterHueGroups {
  std::vector<std::string> labels{};
  std::vector<std::vector<double>> x{};
  std::vector<std::vector<double>> y{};
};

ScatterHueGroups build_scatter_hue_groups(std::span<const std::string> hue_labels,
                                          std::span<const double> x_values,
                                          std::span<const double> y_values) {
  PLUSS_CHECK(
      hue_labels.size() == x_values.size() && hue_labels.size() == y_values.size(),
      "DataFrame::plot().scatter: hue/x/y length mismatch");
  ScatterHueGroups groups;
  std::unordered_map<std::string, std::size_t> index_by_label;
  index_by_label.reserve(hue_labels.size());
  for (const auto i : std::views::iota(std::size_t{0}, hue_labels.size())) {
    const auto [it, inserted] = index_by_label.emplace(hue_labels[i], groups.labels.size());
    if (inserted) {
      groups.labels.push_back(hue_labels[i]);
      groups.x.emplace_back();
      groups.y.emplace_back();
    }
    const auto gi = it->second;
    groups.x[gi].push_back(x_values[i]);
    groups.y[gi].push_back(y_values[i]);
  }
  return groups;
}

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
  return line(y_feature, x_feature, LinePlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::line(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return line(y_features, x_feature, LinePlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::line(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature,
    const LinePlotOptions& options) const {
  const auto value = frame_.features(y_feature);
  const auto resolved_x = resolve_x_feature(x_feature);
  if (resolved_x.has_value()) {
    return value.plot(options_).line(frame_.features(*resolved_x), options);
  }
  return value.plot(options_).line(options);
}

pluss::pl::Studio DataFramePlotAccessor::line(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const LinePlotOptions& options) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().line",
                            [&options](Pane& pane,
                                       const ResolvedPlotAxis& axis,
                                       const std::vector<double>& y_values,
                                       std::string_view name,
                                       std::size_t index) {
                              auto& trace = flash::lineplot(pane, axis.values, y_values);
                              trace.label(std::string(name));
                              style_trace(trace, index, options.alpha, options.line_width);
                            },
                            options_,
                            options.y_limit_method);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_line(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return line(y_features, x_feature, LinePlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::step(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return step(y_feature, x_feature, StepPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::step(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return step(y_features, x_feature, StepPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::step(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature,
    const StepPlotOptions& options) const {
  const auto value = frame_.features(y_feature);
  const auto resolved_x = resolve_x_feature(x_feature);
  if (resolved_x.has_value()) {
    return value.plot(options_).step(frame_.features(*resolved_x), options);
  }
  return value.plot(options_).step(options);
}

pluss::pl::Studio DataFramePlotAccessor::step(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const StepPlotOptions& options) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().step",
                            [&options](Pane& pane,
                                       const ResolvedPlotAxis& axis,
                                       const std::vector<double>& y_values,
                                       std::string_view name,
                                       std::size_t index) {
                              auto& trace = flash::stepplot(pane, axis.values, y_values);
                              trace.label(std::string(name));
                              style_trace(trace, index, options.alpha, options.line_width);
                            },
                            options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_step(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return step(y_features, x_feature, StepPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::area(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return area(y_feature, x_feature, AreaPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::area(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return area(y_features, x_feature, AreaPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::area(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature,
    const AreaPlotOptions& options) const {
  const auto value = frame_.features(y_feature);
  const auto resolved_x = resolve_x_feature(x_feature);
  if (resolved_x.has_value()) {
    return value.plot(options_).area(frame_.features(*resolved_x), options);
  }
  return value.plot(options_).area(options);
}

pluss::pl::Studio DataFramePlotAccessor::area(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const AreaPlotOptions& options) const {
  auto fig = make_xy_figure(frame_, y_features, resolve_x_feature(x_feature), "DataFrame::plot().area",
                            [&options](Pane& pane,
                                       const ResolvedPlotAxis& axis,
                                       const std::vector<double>& y_values,
                                       std::string_view name,
                                       std::size_t index) {
                              auto& trace = flash::areaplot(pane, axis.values, y_values, 0.0);
                              trace.label(std::string(name));
                              style_trace(trace, index, options.alpha, options.line_width);
                            },
                            options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_area(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return area(y_features, x_feature, AreaPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::area_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return area_stacked(y_features, x_feature, AreaStackedPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::area_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const AreaStackedPlotOptions& options) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().area_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().area_stacked", options_);
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1, &options_);
  apply_axis(pane, axis, options_);
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  auto baselines = pluss::Series::full({frame_.ncases()}, 0.0, pluss::codem::ScalarType::Float64);
  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_series(frame_.features(y_name)).reshape({frame_.ncases()});
    require_plot_size(heights, frame_.ncases(), y_name);
    auto tops = baselines.add(heights);
    auto& trace = flash::areaplot(pane, axis.values, tops, 0.0);
    trace.label(std::string(y_name));
    style_trace(trace, feature_index, options.alpha, options.line_width);
    baselines = tops;
    ++feature_index;
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_area_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return area_stacked(y_features, x_feature, AreaStackedPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::scatter(std::string_view x_feature,
                                                 std::string_view y_feature,
                                                 std::optional<std::string_view> hue_feature) const {
  return scatter(x_feature, y_feature, hue_feature, ScatterPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::scatter(
    std::string_view x_feature,
    std::string_view y_feature,
    std::optional<std::string_view> hue_feature,
    const ScatterPlotOptions& options) const {
  const auto resolved_hue = resolve_hue_feature(hue_feature);
  const auto axis = resolve_plot_axis(frame_, x_feature, "DataFrame::plot().scatter", options_);
  const auto y_values = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(y_values, frame_.ncases(), y_feature);

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, resolved_hue.has_value(), &options_);
  apply_axis(pane, axis, options_);
  pane.y_label(std::string(y_feature));

  if (!resolved_hue.has_value()) {
    auto& trace = flash::scatterplot(pane, axis.values, y_values);
    trace.label(std::string(y_feature));
    style_trace(trace, 0, options.alpha, options.line_width);
    return fig;
  }

  const auto hue_thread = frame_.features(*resolved_hue);
  PLUSS_CHECK(thread_supports_hue(hue_thread),
              "DataFrame::plot().scatter: hue feature '" + std::string(*resolved_hue) + "' must be note-like or time-like");
  const auto hue_labels = thread_to_plot_labels(hue_thread, options_);
  require_plot_size(hue_labels, frame_.ncases(), *resolved_hue, "DataFrame::plot().scatter");
  const auto grouped = build_scatter_hue_groups(hue_labels, axis.values, y_values);
  for (const auto group_index : std::views::iota(std::size_t{0}, grouped.labels.size())) {
    auto& trace = flash::scatterplot(pane, grouped.x[group_index], grouped.y[group_index]);
    trace.label(grouped.labels[group_index]);
    style_trace(trace, group_index, options.alpha, options.line_width);
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_scatter(std::string_view x_feature,
                                                      std::string_view y_feature,
                                                      std::optional<std::string_view> hue_feature) const {
  return scatter(x_feature, y_feature, hue_feature, ScatterPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature) const {
  return bar(y_feature, x_feature, BarPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    std::string_view y_feature,
    std::optional<std::string_view> x_feature,
    const BarPlotOptions& options) const {
  const auto value = frame_.features(y_feature);
  const auto resolved_x = resolve_x_feature(x_feature);
  if (resolved_x.has_value()) {
    return value.plot(options_).bar(frame_.features(*resolved_x), options);
  }
  return value.plot(options_).bar(options);
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return bar(y_features, x_feature, BarPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::bar(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const BarPlotOptions& options) const {
  return impl_bar(y_features, x_feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_bar(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return impl_bar(y_features, x_feature, BarPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_bar(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const BarPlotOptions& options) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().bar", options_);
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1, &options_);
  if (options.horizontal) {
    pane.y_label(axis.label);
    if (!axis.labels.empty()) {
      const auto axis_label_views = label_views(axis.labels);
      pane.y_ticks(axis.values, pluss::msharp::RangeView::Of<std::string_view>(axis_label_views));
      apply_y_tick_rotation(pane, options_, std::nullopt);
    }
  } else {
    apply_axis(pane, axis, options_);
    pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));
  }

  const std::size_t group_count = y_features.size();
  std::size_t group_index = 0;
  for (const auto y_name : y_features) {
    auto y_values = thread_to_plot_numeric_values(frame_.features(y_name));
    require_plot_size(y_values, frame_.ncases(), y_name);

    AggregatedBarSeries ci_ready{};
    ci_ready.axis = axis;
    ci_ready.mean = y_values;
    ci_ready.ci_half_width.assign(y_values.size(), 0.0);
    if (options.ci_enabled) {
      ci_ready = aggregate_for_bar_ci(axis, y_values, options.ci_level);
    }

    auto [centers, widths, labels] = grouped_bar_layout(ci_ready.axis, group_index, group_count);
    (void)widths;
    (void)labels;
    flash::BarPlotOptions bar_options;
    bar_options.horizontal = options.horizontal;
    auto& trace = options.horizontal ? flash::barplot(pane, ci_ready.mean, centers, bar_options)
                                     : flash::barplot(pane, centers, ci_ready.mean, bar_options);
    trace.label(std::string(y_name));
    style_trace(trace, group_index, 0.88, 1.0);

    if (options.ci_enabled) {
      if (options.horizontal) {
        std::vector<double> zero_yerr(ci_ready.ci_half_width.size(), 0.0);
        auto& ci = flash::whiskplot(
            pane,
            ci_ready.mean,
            centers,
            zero_yerr,
            zero_yerr,
            ci_ready.ci_half_width,
            ci_ready.ci_half_width);
        ci.label(std::string(y_name) + " CI");
        style_trace(ci, group_index, 0.7, 0.95);
      } else {
        auto& ci = flash::whiskplot(pane, centers, ci_ready.mean, ci_ready.ci_half_width);
        ci.label(std::string(y_name) + " CI");
        style_trace(ci, group_index, 0.7, 0.95);
      }
      ++group_index;
      continue;
    }
    ++group_index;
  }

  if (options.horizontal) {
    pane.x_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::bar_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return bar_stacked(y_features, x_feature, BarStackedPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::bar_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature,
    const BarStackedPlotOptions& options) const {
  PLUSS_CHECK(y_features.size() > 0, "DataFrame::plot().bar_stacked: at least one y feature is required");

  const auto axis = resolve_plot_axis(frame_, resolve_x_feature(x_feature), "DataFrame::plot().bar_stacked", options_);
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, y_features.size() > 1, &options_);
  apply_axis(pane, axis, options_);
  pane.y_label(y_features.size() == 1 ? std::string(*y_features.begin()) : std::string("value"));

  auto baselines = pluss::Series::full({frame_.ncases()}, 0.0, pluss::codem::ScalarType::Float64);
  std::size_t feature_index = 0;
  for (const auto y_name : y_features) {
    auto heights = thread_to_plot_numeric_series(frame_.features(y_name)).reshape({frame_.ncases()});
    require_plot_size(heights, frame_.ncases(), y_name);
    auto tops = baselines.add(heights);
    auto& trace = flash::barplot(pane, axis.values, tops);
    trace.label(std::string(y_name));
    style_trace(trace, feature_index, options.alpha, options.line_width);
    baselines = tops;
    ++feature_index;
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_bar_stacked(
    pluss::msharp::RangeView::Of<std::string_view> y_features,
    std::optional<std::string_view> x_feature) const {
  return bar_stacked(y_features, x_feature, BarStackedPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::box(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return box(features, BoxPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::box(pluss::msharp::RangeView::Of<std::string_view> features,
                                             const BoxPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().box");
  auto fig = box_from_names(frame_, names, options, options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_box(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return box(features, BoxPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::violin(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return violin(features, ViolinPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::violin(pluss::msharp::RangeView::Of<std::string_view> features,
                                                const ViolinPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().violin");
  auto fig = violin_from_names(frame_, names, options, options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::violin(std::string_view value_feature,
                                                std::string_view group_feature) const {
  return violin(value_feature, group_feature, ViolinPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::violin(std::string_view value_feature,
                                                std::string_view group_feature,
                                                const ViolinPlotOptions& options) const {
  const auto y_values = thread_to_plot_numeric_values(frame_.features(value_feature));
  require_plot_size(y_values, frame_.ncases(), value_feature);
  const auto y_series = pluss::make_series(y_values, {static_cast<std::int64_t>(y_values.size())});

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false, &options_);

  const auto group_thread = frame_.features(group_feature);
  if (thread_supports_hue(group_thread)) {
    auto& trace = flash::violinplot(
        pane,
        pluss::make_note(thread_to_plot_labels(group_thread, options_)),
        y_series,
        to_core_violin_options(options));
    trace.label(std::string(value_feature));
    style_trace(trace, 0, options.alpha, options.line_width);
  } else {
    auto group_values = thread_to_plot_numeric_values(group_thread);
    require_plot_size(group_values, frame_.ncases(), group_feature);
    auto& trace = flash::violinplot(
        pane,
        pluss::make_series(group_values, {static_cast<std::int64_t>(group_values.size())}),
        y_series,
        to_core_violin_options(options));
    trace.label(std::string(value_feature));
    style_trace(trace, 0, options.alpha, options.line_width);
  }

  pane.x_label(std::string(group_feature)).y_label(std::string(value_feature));
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_violin(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return violin(features, ViolinPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_violin(std::string_view value_feature,
                                                     std::string_view group_feature) const {
  return violin(value_feature, group_feature, ViolinPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::hist(std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(pluss::msharp::RangeView::Of<std::string_view>{}, options);
}

pluss::pl::Studio DataFramePlotAccessor::hist(pluss::msharp::RangeView::Of<std::string_view> features,
                                              std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::hist(const HistPlotOptions& options) const {
  return hist(pluss::msharp::RangeView::Of<std::string_view>{}, options);
}

pluss::pl::Studio DataFramePlotAccessor::hist(std::string_view feature, const HistPlotOptions& options) const {
  return frame_.features(feature).plot(options_).hist(options);
}

pluss::pl::Studio DataFramePlotAccessor::hist(pluss::msharp::RangeView::Of<std::string_view> features,
                                              const HistPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().hist");
  if (names.size() == 1) {
    auto values = thread_to_plot_numeric_series(frame_.features(names.front())).reshape({frame_.ncases()});
    require_plot_size(values, frame_.ncases(), names.front());
    auto fig = make_studio();
    apply_figure_options(fig);
    auto& pane = fig.pane2();
    setup_pane(pane, false, &options_);
    auto& trace = flash::histplot(pane, values, static_cast<int>(std::max<std::int64_t>(1, resolve_bins(options.bins))));
    trace.label(names.front());
    style_trace(trace, 0, options.alpha, options.line_width);
    pane.x_label(names.front()).y_label("count");
    return fig;
  }

  const auto resolved_bins = static_cast<int>(std::max<std::int64_t>(1, resolve_bins(options.bins)));
  const auto n = static_cast<std::int64_t>(names.size());
  const auto cols = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::ceil(std::sqrt(static_cast<double>(n)))));
  const auto rows = (n + cols - 1) / cols;

  auto fig = make_studio();
  apply_figure_options(fig);
  fig.mosaic(rows, cols, 8.0, 8.0);

  for (std::int64_t i = 0; i < n; ++i) {
    auto& pane = fig.pane2();
    pane.place(i / cols, i % cols);
    setup_pane(pane, false, &options_);
    const auto& name = names[static_cast<std::size_t>(i)];
    auto& trace = flash::histplot(pane,
                                  frame_.features(name).as_series().to(pluss::codem::ScalarType::Float64),
                                  resolved_bins);
    trace.label(name);
    style_trace(trace, static_cast<std::size_t>(i), options.alpha, options.line_width);
    pane.x_label(name).y_label("count");
  }

  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_hist(std::string_view feature, std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  auto values = thread_to_plot_numeric_series(frame_.features(feature)).reshape({frame_.ncases()});
  require_plot_size(values, frame_.ncases(), feature);
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false, &options_);
  auto& trace = flash::histplot(pane, values, static_cast<int>(std::max<std::int64_t>(1, resolve_bins(options.bins))));
  trace.label(std::string(feature));
  style_trace(trace, 0, options.alpha, options.line_width);
  pane.x_label(std::string(feature)).y_label("count");
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_hist(pluss::msharp::RangeView::Of<std::string_view> features,
                                                   std::int64_t bins) const {
  HistPlotOptions options;
  options.bins = bins;
  return hist(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::density(std::string_view feature, std::int64_t points) const {
  DensityPlotOptions options;
  options.points = points;
  return density(feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::density(pluss::msharp::RangeView::Of<std::string_view> features,
                                                 std::int64_t points) const {
  DensityPlotOptions options;
  options.points = points;
  return density(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::density(std::string_view feature, const DensityPlotOptions& options) const {
  return frame_.features(feature).plot(options_).density(options);
}

pluss::pl::Studio DataFramePlotAccessor::density(pluss::msharp::RangeView::Of<std::string_view> features,
                                                 const DensityPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().density");
  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, names.size() > 1, &options_);
  pane.x_label("value").y_label("density");

  const auto resolved_points = std::max<std::int64_t>(16, options.points);
  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = frame_.features(names[i]).as_series().to(pluss::codem::ScalarType::Float64);
    auto& trace = flash::kdeplot(pane, values, resolved_points);
    trace.label(names[i]);
    style_trace(trace, i, options.alpha, options.line_width);
  }
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::impl_density(pluss::msharp::RangeView::Of<std::string_view> features,
                                                      std::int64_t points) const {
  DensityPlotOptions options;
  options.points = points;
  return density(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::pie(std::string_view value_feature,
                                             std::optional<std::string_view> label_feature) const {
  return impl_pie(value_feature, label_feature, PiePlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::pie(
    std::string_view value_feature,
    std::optional<std::string_view> label_feature,
    const PiePlotOptions& options) const {
  const auto values = frame_.features(value_feature);
  if (label_feature.has_value()) {
    return values.plot(options_).pie(frame_.features(*label_feature), options);
  }
  return values.plot(options_).pie(options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_pie(std::string_view value_feature,
                                                  std::optional<std::string_view> label_feature,
                                                  const PiePlotOptions& options) const {
  auto values = thread_to_plot_numeric_values(frame_.features(value_feature));
  require_plot_size(values, frame_.ncases(), value_feature);
  const auto labels = label_feature.has_value() ? thread_to_plot_labels(frame_.features(*label_feature), options_)
                                                : thread_to_plot_labels(frame_.label().values(), options_);
  require_plot_size(labels,
                    frame_.ncases(),
                    label_feature.has_value() ? *label_feature : std::string_view("case"),
                    "DataFrame::plot().pie");

  std::vector<double> filtered_values;
  std::vector<std::string> filtered_labels;
  filtered_values.reserve(values.size());
  filtered_labels.reserve(labels.size());
  for (const auto i : std::views::iota(std::size_t{0}, values.size())) {
    const double value = values[i];
    if (std::isfinite(value) && value > 0.0) {
      filtered_values.push_back(value);
      filtered_labels.push_back(labels[i]);
    }
  }
  PLUSS_CHECK(!filtered_values.empty(),
              "DataFrame::plot().pie: feature '" + std::string(value_feature) + "' has no positive finite values");

  const bool render_label = options.show_label || options.show_percent || options.show_value;
  std::vector<std::string> rendered_labels;
  if (render_label) {
    const int precision = std::clamp(options.precision, 0, 12);
    const double total = std::accumulate(filtered_values.begin(), filtered_values.end(), 0.0);
    rendered_labels.reserve(filtered_values.size());
    for (const auto i : std::views::iota(std::size_t{0}, filtered_values.size())) {
      std::vector<std::string> parts;
      parts.reserve(3);
      if (options.show_label) {
        parts.push_back(filtered_labels[i]);
      }
      if (options.show_percent) {
        std::ostringstream percent;
        percent << std::fixed << std::setprecision(precision)
                << (total > 0.0 ? (100.0 * filtered_values[i] / total) : 0.0) << "%";
        parts.push_back(percent.str());
      }
      if (options.show_value) {
        std::ostringstream value;
        value << std::fixed << std::setprecision(precision) << filtered_values[i];
        parts.push_back(value.str());
      }
      std::string merged;
      for (const auto k : std::views::iota(std::size_t{0}, parts.size())) {
        if (k > 0) {
          merged += options.separator;
        }
        merged += parts[k];
      }
      rendered_labels.push_back(std::move(merged));
    }
  }

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, true, &options_);
  pane.grid(pluss::pl::GridStyle::None).x_label("").y_label("");
  pluss::pl::Trace* trace = nullptr;
  if (render_label) {
    const auto rendered_label_views = label_views(rendered_labels);
    trace = &flash::pieplot(pane,
                            filtered_values,
                            pluss::msharp::RangeView::Of<std::string_view>(rendered_label_views));
  } else {
    trace = &flash::pieplot(pane, filtered_values);
  }
  PLUSS_CHECK(trace != nullptr, "DataFrame::plot().pie: internal trace construction failed");
  trace->label(std::string(value_feature));
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return heatmap(features, HeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const HeatmapPlotOptions& options) const {
  return impl_heatmap(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return impl_heatmap(features, HeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
                                                      const HeatmapPlotOptions& options) const {
  PLUSS_CHECK(features.size() > 0, "DataFrame::plot().heatmap: at least one feature is required");

  std::vector<pluss::Series> columns;
  std::vector<std::string> y_labels;
  columns.reserve(features.size());
  y_labels.reserve(features.size());
  for (const auto feature : features) {
    auto feature_values = thread_to_plot_numeric_series(frame_.features(feature)).reshape({frame_.ncases()});
    PLUSS_CHECK(feature_values.numel() == frame_.ncases(),
                "DataFrame::plot().heatmap: feature '" + std::string(feature) + "' length mismatch with cases");
    columns.push_back(std::move(feature_values));
    y_labels.emplace_back(feature);
  }

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false, &options_);
  const auto x_positions = positional_plot_x(frame_.ncases());
  const auto y_positions = positional_plot_x(static_cast<std::int64_t>(y_labels.size()));
  auto z = columns_to_matrix_series(columns, frame_.ncases(), false).transpose(0, 1).contiguous();
  auto& trace = flash::heatmap(pane, x_positions, y_positions, z, to_flash_options(options));
  trace.label("value").shade(pluss::pl::ShadeMode::Smooth);
  auto case_labels = labels_for_cases(frame_, "DataFrame::plot().heatmap", options_);
  const auto case_label_views = label_views(case_labels);
  const auto y_label_views = label_views(y_labels);
  pane.x_ticks(positional_plot_x(frame_.ncases()),
               pluss::msharp::RangeView::Of<std::string_view>(case_label_views));
  pane.y_ticks(positional_plot_x(static_cast<std::int64_t>(y_labels.size())),
               pluss::msharp::RangeView::Of<std::string_view>(y_label_views));
  apply_x_tick_rotation(pane, options_, false);
  apply_y_tick_rotation(pane, options_, options.borderless ? std::nullopt : std::optional<double>{30.0});
  pane.x_label("case").y_label("feature");
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(std::string_view case_feature,
                                                 std::string_view feature_feature,
                                                 std::string_view value_feature,
                                                 DataFrame::GroupOp op) const {
  return heatmap(case_feature, feature_feature, value_feature, op, HeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::heatmap(std::string_view case_feature,
                                                 std::string_view feature_feature,
                                                 std::string_view value_feature,
                                                 DataFrame::GroupOp op,
                                                 const HeatmapPlotOptions& options) const {
  return impl_heatmap(case_feature, feature_feature, value_feature, op, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(std::string_view case_feature,
                                                      std::string_view feature_feature,
                                                      std::string_view value_feature,
                                                      DataFrame::GroupOp op) const {
  return impl_heatmap(case_feature, feature_feature, value_feature, op, HeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_heatmap(std::string_view case_feature,
                                                      std::string_view feature_feature,
                                                      std::string_view value_feature,
                                                      DataFrame::GroupOp op,
                                                      const HeatmapPlotOptions& options) const {
  auto pivoted = frame_.pivot_table(case_feature, feature_feature, value_feature, op, true, false);
  const auto& pivot_features = pivoted.features();
  PLUSS_CHECK(!pivot_features.empty(),
              "DataFrame::plot().heatmap: pivot produced no columns for feature '" + std::string(feature_feature) + "'");

  std::vector<pluss::Series> columns;
  columns.reserve(pivot_features.size());
  std::transform(
      pivot_features.begin(),
      pivot_features.end(),
      std::back_inserter(columns),
      [&](const std::string& name) {
        auto column_values = thread_to_plot_numeric_series(pivoted.features(name)).reshape({pivoted.ncases()});
        PLUSS_CHECK(column_values.numel() == pivoted.ncases(),
                    "DataFrame::plot().heatmap: feature '" + name + "' length mismatch with cases");
        return column_values;
      });

  auto fig = make_studio();
  apply_figure_options(fig);
  auto& pane = fig.pane2();
  setup_pane(pane, false, &options_);
  const auto x_positions = positional_plot_x(static_cast<std::int64_t>(pivot_features.size()));
  const auto y_positions = positional_plot_x(pivoted.ncases());
  auto z = columns_to_matrix_series(columns, pivoted.ncases(), true);
  auto& trace = flash::heatmap(pane, x_positions, y_positions, z, to_flash_options(options));
  trace.label(std::string(value_feature)).shade(pluss::pl::ShadeMode::Smooth);
  std::vector<std::string> feature_labels(pivot_features.begin(), pivot_features.end());
  auto case_labels = labels_for_cases(pivoted, "DataFrame::plot().heatmap", options_);
  const auto feature_label_views = label_views(feature_labels);
  const auto case_label_views = label_views(case_labels);
  pane.x_ticks(positional_plot_x(static_cast<std::int64_t>(pivot_features.size())),
               pluss::msharp::RangeView::Of<std::string_view>(feature_label_views));
  pane.y_ticks(positional_plot_x(pivoted.ncases()),
               pluss::msharp::RangeView::Of<std::string_view>(case_label_views));
  apply_x_tick_rotation(pane, options_, false);
  apply_y_tick_rotation(pane, options_, options.borderless ? std::nullopt : std::optional<double>{30.0});
  pane.x_label(std::string(feature_feature)).y_label(std::string(case_feature));
  return fig;
}

struct PivotGrid3D {
  std::vector<double> x_axis{};
  std::vector<double> y_axis{};
  pluss::Series z{};
  std::int64_t rows{0};
  std::int64_t cols{0};
};

PivotGrid3D build_pivot_grid_3d(const DataFrame& pivoted, std::string_view where) {
  PivotGrid3D out;
  out.rows = pivoted.ncases();
  out.cols = static_cast<std::int64_t>(pivoted.nfeatures());
  PLUSS_CHECK(out.rows > 0, std::string(where) + ": pivot produced zero rows");
  PLUSS_CHECK(out.cols > 0, std::string(where) + ": pivot produced zero columns");

  out.y_axis = thread_to_plot_numeric_values(pivoted.label().values());
  std::size_t y_index = 0;
  std::for_each(out.y_axis.begin(), out.y_axis.end(), [&](double& value) {
    if (!std::isfinite(value)) {
      value = static_cast<double>(y_index);
    }
    ++y_index;
  });

  const auto feature_names = pivoted.features();
  out.x_axis.reserve(feature_names.size());
  std::size_t x_index = 0;
  std::transform(
      feature_names.begin(),
      feature_names.end(),
      std::back_inserter(out.x_axis),
      [&](const std::string& name) {
        char* end = nullptr;
        const double parsed = std::strtod(name.c_str(), &end);
        const bool ok = (end != nullptr) && (end != name.c_str()) && (*end == '\0');
        const double fallback = static_cast<double>(x_index);
        ++x_index;
        return ok ? parsed : fallback;
      });

  std::vector<pluss::Series> columns;
  columns.reserve(feature_names.size());
  std::transform(
      feature_names.begin(),
      feature_names.end(),
      std::back_inserter(columns),
      [&](const std::string& name) {
        auto values = thread_to_plot_numeric_series(pivoted.features(name)).reshape({out.rows});
        PLUSS_CHECK(values.numel() == out.rows,
                    std::string(where) + ": feature '" + name + "' length mismatch with cases");
        return values;
      });
  out.z = columns_to_matrix_series(columns, out.rows, true);
  return out;
}

pluss::pl::Studio DataFramePlotAccessor::corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return corr_heatmap(features, CorrHeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return impl_corr_heatmap(features, CorrHeatmapPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::corr_heatmap(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const CorrHeatmapPlotOptions& options) const {
  return impl_corr_heatmap(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> features,
                                                           const CorrHeatmapPlotOptions& options) const {
  const auto names = correlation_feature_names(frame_, features);
  auto fig = corr_heatmap_from_names(frame_, names, options, options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return scatter_matrix(features, ScatterMatrixPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::impl_scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return impl_scatter_matrix(features, ScatterMatrixPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::scatter_matrix(
    pluss::msharp::RangeView::Of<std::string_view> features,
    ScatterMatrixTriangle triangle) const {
  ScatterMatrixPlotOptions options;
  options.pair.triangle = triangle;
  return impl_scatter_matrix(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::scatter_matrix(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const ScatterMatrixPlotOptions& options) const {
  return impl_scatter_matrix(features, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> features,
                                                             const ScatterMatrixPlotOptions& options) const {
  const auto scoped = features.size() > 0 ? frame_.features(features) : frame_;
  const auto series_only = scoped.feataxis().series();
  const auto names = series_only.features();
  auto fig = scatter_matrix_from_names(series_only, names, resolve_scatter_matrix_pair_options(options), options_);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::parallel(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const ParallelPlotOptions& options) const {
  return impl_parallel(features, class_feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_parallel(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const ParallelPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().parallel");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().parallel");
  auto flash_options = to_flash_options(options);
  if (flash_options.labels.empty()) {
    flash_options.labels = names;
  }
  auto fig = make_studio();
  if (class_feature.has_value()) {
    const auto classes = thread_to_plot_note(
        frame_.features(*class_feature),
        "DataFrame::plot().parallel",
        *class_feature);
    flash::parallelplot(fig, columns, classes, flash_options);
  } else {
    flash::parallelplot(fig, columns, flash_options);
  }
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::andrews_curves(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const AndrewsCurvesPlotOptions& options) const {
  return impl_andrews_curves(features, class_feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_andrews_curves(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const AndrewsCurvesPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().andrews_curves");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().andrews_curves");
  auto flash_options = to_flash_options(options);
  if (flash_options.labels.empty()) {
    flash_options.labels = names;
  }
  auto fig = make_studio();
  if (class_feature.has_value()) {
    const auto classes = thread_to_plot_note(
        frame_.features(*class_feature),
        "DataFrame::plot().andrews_curves",
        *class_feature);
    flash::andrews_curves(fig, columns, classes, flash_options);
  } else {
    flash::andrews_curves(fig, columns, flash_options);
  }
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::radviz(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const RadvizPlotOptions& options) const {
  return impl_radviz(features, class_feature, options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_radviz(
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<std::string_view> class_feature,
    const RadvizPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().radviz");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().radviz");
  auto flash_options = to_flash_options(options);
  if (flash_options.labels.empty()) {
    flash_options.labels = names;
  }
  auto fig = make_studio();
  if (class_feature.has_value()) {
    const auto classes = thread_to_plot_note(
        frame_.features(*class_feature),
        "DataFrame::plot().radviz",
        *class_feature);
    flash::radviz(fig, columns, classes, flash_options);
  } else {
    flash::radviz(fig, columns, flash_options);
  }
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::lag(
    std::string_view feature,
    const LagPlotOptions& options) const {
  return frame_.features(feature).plot(options_).lag(options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_lag(
    std::string_view feature,
    const LagPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, {feature}, "DataFrame::plot().lag");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().lag");
  auto fig = make_studio();
  flash::lagplot(fig, columns.front(), to_flash_options(options));
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::act(
    std::string_view feature,
    const ActPlotOptions& options) const {
  return frame_.features(feature).plot(options_).act(options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_act(
    std::string_view feature,
    const ActPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, {feature}, "DataFrame::plot().act");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().act");
  auto fig = make_studio();
  flash::actplot(fig, columns.front(), to_flash_options(options));
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::bootstrap(
    std::string_view feature,
    const BootstrapPlotOptions& options) const {
  return frame_.features(feature).plot(options_).bootstrap(options);
}

pluss::pl::Studio DataFramePlotAccessor::impl_bootstrap(
    std::string_view feature,
    const BootstrapPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, {feature}, "DataFrame::plot().bootstrap");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().bootstrap");
  auto fig = make_studio();
  flash::bootstrapplot(fig, columns.front(), to_flash_options(options));
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::brief(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const BriefPlotOptions& options) const {
  return impl_brief(features, options);
}

pluss::pl::Studio& DataFramePlotAccessor::brief(
    pluss::pl::Studio& figure,
    pluss::msharp::RangeView::Of<std::string_view> features,
    const BriefPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().brief");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().brief");
  const auto name_views = label_views(names);
  flash::summaryplot(
      figure,
      columns,
      pluss::msharp::RangeView::Of<std::string_view>(name_views),
      to_flash_options(options));
  return figure;
}

pluss::pl::Studio DataFramePlotAccessor::impl_brief(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const BriefPlotOptions& options) const {
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().brief");
  auto columns = numeric_series_columns(frame_, names, "DataFrame::plot().brief");
  auto fig = make_studio();
  const auto name_views = label_views(names);
  flash::summaryplot(
      fig,
      columns,
      pluss::msharp::RangeView::Of<std::string_view>(name_views),
      to_flash_options(options));
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlotAccessor::auto_plot(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return auto_plot(features, AutoPlotOptions{});
}

pluss::pl::Studio DataFramePlotAccessor::auto_plot(
    pluss::msharp::RangeView::Of<std::string_view> features,
    const AutoPlotOptions&) const {
  return impl_auto_plot(features);
}

pluss::pl::Studio DataFramePlotAccessor::impl_auto_plot(pluss::msharp::RangeView::Of<std::string_view> features) const {
  const bool explicit_features = features.size() > 0;
  const auto names = numeric_feature_names(frame_, features, "DataFrame::plot().auto_plot");
  if (names.size() == 1) {
    return frame_.features(names.front()).plot(options_).auto_plot();
  }
  if (!explicit_features && names.size() == 2) {
    const auto hue_names = hue_feature_names(frame_);
    if (!hue_names.empty()) {
      return scatter(names[0], names[1], hue_names.front());
    }
  }
  if (names.size() <= 3 && thread_prefers_labeled_axis(frame_.label().values()) && all_nonnegative_columns(frame_, names)) {
    if (frame_.ncases() <= 12) {
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
  if (names.size() <= 5 && frame_.ncases() <= 256) {
    auto fig = scatter_matrix_from_names(
        frame_, names, resolve_scatter_matrix_pair_options(ScatterMatrixPlotOptions{}), options_);
    apply_figure_options(fig);
    return fig;
  }
  const CorrHeatmapPlotOptions corr_options{};
  auto fig = corr_heatmap_from_names(frame_, names, corr_options, options_);
  apply_figure_options(fig);
  return fig;
}

DataFramePlot3DAccessor::DataFramePlot3DAccessor(DataFrame frame, DataFramePlotOptions options)
    : frame_(std::move(frame)), options_(std::move(options)) {}

void DataFramePlot3DAccessor::apply_figure_options(pluss::pl::Studio& figure) const {
  const double width = options_.width.value_or(figure.extent().width);
  const double height = options_.height.value_or(figure.extent().height);
  figure.size(width, height);
  if (options_.use_tight_layout) {
    figure.tight_layout(true, options_.tight_layout_padding);
  }
}

pluss::pl::Studio DataFramePlot3DAccessor::scatter(std::string_view x_feature,
                                                   std::string_view y_feature,
                                                   std::string_view z_feature,
                                                   std::optional<std::string_view> size_feature,
                                                   std::optional<std::string_view> color_feature,
                                                   const Scatter3DPlotOptions& options) const {
  auto fig = make_studio();
  auto& pane = fig.pane3("scatter3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);

  const auto n = frame_.ncases();
  const auto x = thread_to_plot_numeric_values(frame_.features(x_feature));
  const auto y = thread_to_plot_numeric_values(frame_.features(y_feature));
  const auto z = thread_to_plot_numeric_values(frame_.features(z_feature));
  require_plot_size(x, n, x_feature);
  require_plot_size(y, n, y_feature);
  require_plot_size(z, n, z_feature);

  Trace* trace = nullptr;
  if (size_feature.has_value()) {
    const auto s = thread_to_plot_numeric_values(frame_.features(*size_feature));
    require_plot_size(s, n, *size_feature);
    if (color_feature.has_value()) {
      const auto c = thread_to_plot_numeric_values(frame_.features(*color_feature));
      require_plot_size(c, n, *color_feature);
      trace = &flash::cloud3(pane, x, y, z, s, c).label("scatter3d");
    } else {
      trace = &flash::cloud3(pane, x, y, z, s).label("scatter3d");
    }
  } else if (color_feature.has_value()) {
    const auto c = thread_to_plot_numeric_values(frame_.features(*color_feature));
    require_plot_size(c, n, *color_feature);
    std::vector<double> default_size(static_cast<std::size_t>(n), options.default_marker_size);
    trace = &flash::cloud3(pane, x, y, z, default_size, c).label("scatter3d");
  } else {
    trace = &flash::cloud3(pane, x, y, z).label("scatter3d");
  }
  if (trace != nullptr) {
    plot::detail::apply_3d_trace_options(*trace, options.trace);
  }

  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlot3DAccessor::line(std::string_view x_feature,
                                                std::string_view y_feature,
                                                std::string_view z_feature,
                                                const Line3DPlotOptions& options) const {
  auto fig = make_studio();
  auto& pane = fig.pane3("line3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);

  const auto n = frame_.ncases();
  const auto x = thread_to_plot_numeric_values(frame_.features(x_feature));
  const auto y = thread_to_plot_numeric_values(frame_.features(y_feature));
  const auto z = thread_to_plot_numeric_values(frame_.features(z_feature));
  require_plot_size(x, n, x_feature);
  require_plot_size(y, n, y_feature);
  require_plot_size(z, n, z_feature);

  auto& trace = flash::path3(pane, x, y, z).label("line3d");
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlot3DAccessor::hist(std::string_view x_feature,
                                                std::string_view y_feature,
                                                const Hist3DPlotOptions& options) const {
  auto fig = make_studio();
  auto& pane = fig.pane3("hist3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(options.normalize ? "density" : "count");
  plot::detail::apply_3d_scene_options(pane, options.scene);

  const auto n = frame_.ncases();
  const auto x = thread_to_plot_numeric_values(frame_.features(x_feature));
  const auto y = thread_to_plot_numeric_values(frame_.features(y_feature));
  require_plot_size(x, n, x_feature);
  require_plot_size(y, n, y_feature);

  pluss::pl::flash::Hist3DOptions hist_options;
  hist_options.bins_x = options.bins_x;
  hist_options.bins_y = options.bins_y;
  hist_options.normalize = options.normalize;
  hist_options.smooth = options.smooth;
  hist_options.smooth_passes = options.smooth_passes;
  hist_options.as_surface = options.as_surface;
  hist_options.as_wireframe = options.as_wireframe;

  auto& trace = flash::hist3d(pane, x, y, hist_options).label(options.normalize ? "density" : "count");
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlot3DAccessor::surface(std::string_view x_feature,
                                                   std::string_view y_feature,
                                                   std::string_view z_feature,
                                                   DataFrame::GroupOp op,
                                                   const Surface3DPlotOptions& options) const {
  auto pivoted = frame_.pivot_table(y_feature, x_feature, z_feature, op, true, false);
  auto grid = build_pivot_grid_3d(pivoted, "DataFrame::plot3d().surface");

  auto fig = make_studio();
  auto& pane = fig.pane3("surface3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);
  auto& trace = flash::sheet(pane, grid.x_axis, grid.y_axis, grid.z).label("surface3d");
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio DataFramePlot3DAccessor::wireframe(std::string_view x_feature,
                                                     std::string_view y_feature,
                                                     std::string_view z_feature,
                                                     DataFrame::GroupOp op,
                                                     const Wireframe3DPlotOptions& options) const {
  auto pivoted = frame_.pivot_table(y_feature, x_feature, z_feature, op, true, false);
  auto grid = build_pivot_grid_3d(pivoted, "DataFrame::plot3d().wireframe");

  auto fig = make_studio();
  auto& pane = fig.pane3("wireframe3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);
  auto& trace = flash::lattice(pane, grid.x_axis, grid.y_axis, grid.z).label("wireframe3d");
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

DataFramePlotAccessor DataFrame::plot() const {
  return DataFramePlotAccessor(*this);
}

DataFramePlotAccessor DataFrame::plot(const DataFramePlotOptions& options) const {
  return DataFramePlotAccessor(*this, options);
}

DataFramePlot3DAccessor DataFrame::plot3d() const {
  return DataFramePlot3DAccessor(*this);
}

DataFramePlot3DAccessor DataFrame::plot3d(const DataFramePlotOptions& options) const {
  return DataFramePlot3DAccessor(*this, options);
}

} // namespace pluss::table
