#include "table/plot/AtlasPlot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <numeric>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "CodeM/core/Errors.h"
#include "table/plot/detail/Plot3DConfig.h"

namespace pluss::table {

namespace {

using Studio = pluss::pl::Studio;
using Pane = pluss::pl::Pane;
using Rgba = pluss::pl::Rgba;
namespace flash = pluss::pl::flash;

constexpr std::string_view kAtlasPlotName = "atlas";

Studio make_studio() {
  auto fig = pluss::pl::studio(std::string(kAtlasPlotName));
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

std::string compact_number_label(double value) {
  const double rounded = std::round(value);
  if (std::abs(value - rounded) < 1e-9) {
    return std::to_string(static_cast<std::int64_t>(rounded));
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6) << value;
  auto text = oss.str();
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

std::int64_t normalize_frame_index(std::int64_t idx, std::int64_t depth) {
  PLUSS_CHECK(depth > 0, "Atlas::plot().frames: atlas has no frames");
  std::int64_t out = idx;
  if (out < 0) {
    out += depth;
  }
  PLUSS_CHECK(out >= 0 && out < depth, "Atlas::plot().frames: frame index out of bounds");
  return out;
}

std::vector<std::int64_t> normalize_frame_indices(pluss::msharp::RangeView::Int indices, std::int64_t depth) {
  PLUSS_CHECK(indices.size() != 0, "Atlas::plot().frames: at least one frame index is required");
  std::vector<std::int64_t> out;
  out.reserve(indices.size());
  std::vector<std::uint8_t> seen(static_cast<std::size_t>(depth), static_cast<std::uint8_t>(0));
  for (const auto idx : indices) {
    const auto normalized = normalize_frame_index(idx, depth);
    auto& visited = seen[static_cast<std::size_t>(normalized)];
    if (visited == static_cast<std::uint8_t>(0)) {
      visited = static_cast<std::uint8_t>(1);
      out.push_back(normalized);
    }
  }
  PLUSS_CHECK(!out.empty(), "Atlas::plot().frames: no valid frame indices selected");
  return out;
}

std::vector<std::int64_t> depth_positions(std::int64_t depth, const AtlasDepthAxisOptions& options) {
  PLUSS_CHECK(depth > 0, "Atlas::plot(): atlas has no frames");
  const auto step = std::max<std::int64_t>(1, options.step);
  std::vector<std::int64_t> positions;
  positions.reserve(static_cast<std::size_t>((depth + step - 1) / step));
  if (options.order == AtlasDepthOrder::Ascending) {
    for (auto i = std::int64_t{0}; i < depth; i += step) {
      positions.push_back(i);
    }
    return positions;
  }
  for (auto i = depth - 1; i >= 0; i -= step) {
    positions.push_back(i);
    if (i < step) {
      break;
    }
  }
  return positions;
}

std::vector<double> positional_axis(std::int64_t n) {
  PLUSS_CHECK(n >= 0, "Atlas::plot: axis length must be non-negative");
  std::vector<double> values(static_cast<std::size_t>(n), 0.0);
  std::iota(values.begin(), values.end(), 0.0);
  return values;
}

bool thread_prefers_labels(const Thread& t) {
  return t.kind() == ThreadKind::Note || t.kind() == ThreadKind::Timeline || t.kind() == ThreadKind::TimeIndex;
}

std::vector<std::string> thread_to_labels(const Thread& thread) {
  const Thread note_like = (thread.kind() == ThreadKind::Note) ? thread : thread.cast(ThreadCastTarget::Note);
  const auto& note = note_like.as_note();
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(note.numel()));
  note.for_each_flat([&](std::string_view s) { labels.emplace_back(s); });
  return labels;
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

pluss::Series thread_to_numeric_series(const Thread& t, std::string_view where, std::string_view feature_name) {
  Thread numeric_thread = t;
  switch (t.kind()) {
    case ThreadKind::Series:
      break;
    case ThreadKind::Timeline:
      numeric_thread = Thread(t.as_timeline().epoch_nanos(), t.validity_mask());
      break;
    case ThreadKind::TimeIndex:
      numeric_thread = Thread(t.as_timeindex().timeline().epoch_nanos(), t.validity_mask());
      break;
    case ThreadKind::Note:
      numeric_thread = t.cast(ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);
      break;
    default:
      PLUSS_FAIL(std::string(where) + ": feature '" + std::string(feature_name) + "' is not numeric/threadable");
  }

  auto values = numeric_thread.as_series().to(pluss::codem::ScalarType::Float64);
  if (numeric_thread.has_validity_mask()) {
    const auto nan_fill = pluss::Series::full(
        {values.numel()},
        std::numeric_limits<double>::quiet_NaN(),
        pluss::codem::ScalarType::Float64);
    values = values.where(numeric_thread.validity_mask()->logical_not(), nan_fill);
  }
  return values.reshape({values.numel()});
}

std::vector<double> thread_to_numeric_values(const Thread& t,
                                             std::string_view where,
                                             std::string_view feature_name) {
  return thread_to_numeric_series(t, where, feature_name).to_vec<double>();
}

void require_size(std::int64_t actual, std::int64_t expected, std::string_view where, std::string_view what) {
  PLUSS_CHECK(
      actual == expected,
      std::string(where) + ": " + std::string(what) + " length mismatch (expected " + std::to_string(expected)
          + ", got " + std::to_string(actual) + ")");
}

void require_min_size(std::int64_t actual, std::int64_t minimum, std::string_view where, std::string_view what) {
  PLUSS_CHECK(
      actual >= minimum,
      std::string(where) + ": " + std::string(what) + " length mismatch (minimum " + std::to_string(minimum)
          + ", got " + std::to_string(actual) + ")");
}

std::vector<std::string> depth_legend_labels(const std::optional<Thread>& depth_labels,
                                             std::span<const std::int64_t> positions,
                                             std::string_view where) {
  const auto depth = static_cast<std::int64_t>(positions.size());
  PLUSS_CHECK(depth > 0, std::string(where) + ": empty depth selection");
  if (!depth_labels.has_value()) {
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(depth));
    for (const auto p : positions) {
      labels.push_back("frame " + std::to_string(p));
    }
    return labels;
  }

  if (thread_prefers_labels(*depth_labels)) {
    const auto source = thread_to_labels(*depth_labels);
    require_min_size(
        static_cast<std::int64_t>(source.size()),
        *std::max_element(positions.begin(), positions.end()) + 1,
        where,
        "depth labels");
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(depth));
    for (const auto p : positions) {
      labels.push_back(source[static_cast<std::size_t>(p)]);
    }
    return labels;
  }

  const auto values = thread_to_numeric_values(*depth_labels, where, "depth axis");
  require_min_size(
      static_cast<std::int64_t>(values.size()),
      *std::max_element(positions.begin(), positions.end()) + 1,
      where,
      "depth axis");
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(depth));
  for (const auto p : positions) {
    labels.push_back(compact_number_label(values[static_cast<std::size_t>(p)]));
  }
  return labels;
}

struct AtlasGrid3D {
  std::vector<double> x_axis{};
  std::vector<std::string> x_labels{};
  std::vector<double> y_axis{};
  std::vector<std::string> y_labels{};
  pluss::Series z{};
};

struct DepthAxis2D {
  std::vector<double> values{};
  std::vector<std::string> labels{};
};

double quantile_sorted(const std::vector<double>& sorted, double q) {
  PLUSS_CHECK(!sorted.empty(), "Atlas::plot: quantile on empty data");
  if (q <= 0.0) {
    return sorted.front();
  }
  if (q >= 1.0) {
    return sorted.back();
  }
  const double pos = q * static_cast<double>(sorted.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo == hi) {
    return sorted[lo];
  }
  const double frac = pos - static_cast<double>(lo);
  return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

std::vector<double> finite_sorted(std::vector<double> values) {
  std::vector<double> out;
  out.reserve(values.size());
  for (const double value : values) {
    if (std::isfinite(value)) {
      out.push_back(value);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

double reduce_statistic(const std::vector<double>& values, AtlasDepthStatistic stat) {
  const auto sorted = finite_sorted(values);
  if (sorted.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  switch (stat) {
    case AtlasDepthStatistic::Mean: {
      const double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
      return sum / static_cast<double>(sorted.size());
    }
    case AtlasDepthStatistic::Median:
      return quantile_sorted(sorted, 0.5);
    case AtlasDepthStatistic::Min:
      return sorted.front();
    case AtlasDepthStatistic::Max:
      return sorted.back();
  }
  PLUSS_FAIL("Atlas::plot().depth: unsupported reduction statistic");
}

DepthAxis2D build_depth_axis(const std::optional<Thread>& depth_labels,
                             std::span<const std::int64_t> positions,
                             std::string_view where) {
  const auto depth = static_cast<std::int64_t>(positions.size());
  PLUSS_CHECK(depth > 0, std::string(where) + ": empty depth selection");
  DepthAxis2D axis;
  if (!depth_labels.has_value()) {
    axis.values.reserve(static_cast<std::size_t>(depth));
    for (const auto p : positions) {
      axis.values.push_back(static_cast<double>(p));
    }
    return axis;
  }
  if (thread_prefers_labels(*depth_labels)) {
    axis.values = positional_axis(depth);
    const auto source = thread_to_labels(*depth_labels);
    require_min_size(
        static_cast<std::int64_t>(source.size()),
        *std::max_element(positions.begin(), positions.end()) + 1,
        where,
        "depth labels");
    axis.labels.reserve(static_cast<std::size_t>(depth));
    for (const auto p : positions) {
      axis.labels.push_back(source[static_cast<std::size_t>(p)]);
    }
    return axis;
  }
  const auto source = thread_to_numeric_values(*depth_labels, where, "depth axis");
  require_min_size(
      static_cast<std::int64_t>(source.size()),
      *std::max_element(positions.begin(), positions.end()) + 1,
      where,
      "depth axis");
  axis.values.reserve(static_cast<std::size_t>(depth));
  for (const auto p : positions) {
    axis.values.push_back(source[static_cast<std::size_t>(p)]);
  }
  return axis;
}

std::vector<std::string> default_triplet_features(const std::vector<DataFrame>& frames) {
  PLUSS_CHECK(!frames.empty(), "Atlas::plot().triplot: atlas has no frames");
  const auto numeric = frames.front().series_features();
  auto names = numeric.feature_names();
  PLUSS_CHECK(
      static_cast<std::int64_t>(names.size()) >= 3,
      "Atlas::plot().triplot: requires at least three numeric series features");
  return names;
}

std::vector<std::string> resolve_triplet_features(pluss::msharp::RangeView::Of<std::string_view> requested,
                                                  const std::vector<DataFrame>& frames) {
  if (requested.size() == 0) {
    return default_triplet_features(frames);
  }
  PLUSS_CHECK(!frames.empty(), "Atlas::plot().triplot: atlas has no frames");
  const auto numeric_names = frames.front().series_features().feature_names();
  std::unordered_set<std::string> available_set;
  available_set.reserve(numeric_names.size());
  for (const auto& name : numeric_names) {
    available_set.insert(name);
  }
  std::vector<std::string> features;
  features.reserve(requested.size());
  std::unordered_set<std::string> selected_set;
  selected_set.reserve(requested.size());
  for (const auto name : requested) {
    const std::string feature(name);
    PLUSS_CHECK(
        available_set.contains(feature),
        "Atlas::plot().triplot: feature '" + feature + "' is missing or not numeric");
    if (selected_set.insert(feature).second) {
      features.push_back(feature);
    }
  }
  PLUSS_CHECK(
      static_cast<std::int64_t>(features.size()) >= 3,
      "Atlas::plot().triplot: requires at least three selected features");
  return features;
}

std::vector<std::string> resolve_depth_features(pluss::msharp::RangeView::Of<std::string_view> requested,
                                                const std::vector<DataFrame>& frames,
                                                std::int64_t max_features,
                                                std::string_view where) {
  PLUSS_CHECK(!frames.empty(), std::string(where) + ": atlas has no frames");
  const auto numeric = frames.front().series_features();
  const auto numeric_names = numeric.feature_names();
  std::unordered_set<std::string> available_set;
  available_set.reserve(numeric_names.size());
  for (const auto& name : numeric_names) {
    available_set.insert(name);
  }

  std::vector<std::string> features;
  if (requested.size() == 0) {
    features = numeric_names;
  } else {
    features.reserve(requested.size());
    std::unordered_set<std::string> selected_set;
    selected_set.reserve(requested.size());
    for (const auto name_view : requested) {
      const std::string name(name_view);
      if (!selected_set.insert(name).second) {
        continue;
      }
      PLUSS_CHECK(
          available_set.contains(name),
          std::string(where) + ": feature '" + name + "' is missing or not numeric");
      features.push_back(name);
    }
  }

  PLUSS_CHECK(!features.empty(), std::string(where) + ": requires at least one numeric feature");
  if (max_features > 0 && static_cast<std::int64_t>(features.size()) > max_features) {
    features.resize(static_cast<std::size_t>(max_features));
  }
  return features;
}

std::vector<double> depth_statistic_values(const std::vector<DataFrame>& frames,
                                           std::span<const std::int64_t> positions,
                                           std::string_view feature,
                                           AtlasDepthStatistic statistic,
                                           std::string_view where) {
  PLUSS_CHECK(!positions.empty(), std::string(where) + ": empty depth selection");
  const auto ncases = frames[static_cast<std::size_t>(positions.front())].ncases();
  PLUSS_CHECK(ncases > 0, std::string(where) + ": frame has zero cases");

  std::vector<double> values;
  values.reserve(positions.size());
  for (const auto p : positions) {
    const auto& frame = frames[static_cast<std::size_t>(p)];
    require_size(frame.ncases(), ncases, where, "case axis");
    PLUSS_CHECK(
        frame.has_feature(feature),
        std::string(where) + ": missing feature '" + std::string(feature) + "'");
    const auto frame_values = thread_to_numeric_values(frame.features(feature), where, feature);
    values.push_back(reduce_statistic(frame_values, statistic));
  }
  return values;
}

std::vector<pluss::Series> flatten_feature_columns(const std::vector<DataFrame>& frames,
                                                   const std::vector<std::string>& features,
                                                   std::string_view where) {
  PLUSS_CHECK(!frames.empty(), std::string(where) + ": atlas has no frames");
  const auto depth = static_cast<std::int64_t>(frames.size());
  const auto ncases = frames.front().ncases();
  PLUSS_CHECK(ncases > 0, std::string(where) + ": frame has zero cases");

  std::vector<std::vector<double>> flattened(features.size());
  for (auto& column : flattened) {
    column.reserve(static_cast<std::size_t>(depth * ncases));
  }

  for (const auto& frame : frames) {
    require_size(frame.ncases(), ncases, where, "case axis");
    for (const auto i : std::views::iota(std::size_t{0}, features.size())) {
      const auto& feature = features[i];
      PLUSS_CHECK(frame.has_feature(feature), std::string(where) + ": missing feature '" + feature + "'");
      const auto values = thread_to_numeric_values(frame.features(feature), where, feature);
      require_size(static_cast<std::int64_t>(values.size()), ncases, where, feature);
      auto& dest = flattened[i];
      dest.insert(dest.end(), values.begin(), values.end());
    }
  }

  std::vector<pluss::Series> columns;
  columns.reserve(features.size());
  for (auto& values : flattened) {
    columns.push_back(pluss::make_series(values));
  }
  return columns;
}

AtlasGrid3D build_depth_case_grid(const std::vector<DataFrame>& frames,
                                  const std::optional<Thread>& depth_labels,
                                  std::span<const std::int64_t> positions,
                                  std::string_view feature,
                                  std::string_view where) {
  PLUSS_CHECK(!positions.empty(), std::string(where) + ": empty depth selection");

  const auto depth = static_cast<std::int64_t>(positions.size());
  const auto ncases = frames[static_cast<std::size_t>(positions.front())].ncases();
  PLUSS_CHECK(ncases > 0, std::string(where) + ": frame has zero cases");

  for (const auto p : positions) {
    const auto& frame = frames[static_cast<std::size_t>(p)];
    require_size(frame.ncases(), ncases, where, "case axis");
    PLUSS_CHECK(frame.has_feature(feature),
                std::string(where) + ": missing feature '" + std::string(feature) + "' in one or more frames");
  }

  AtlasGrid3D out;
  const auto& case_thread = frames[static_cast<std::size_t>(positions.front())].cases();
  if (thread_prefers_labels(case_thread)) {
    out.x_axis = positional_axis(ncases);
    out.x_labels = thread_to_labels(case_thread);
    require_size(static_cast<std::int64_t>(out.x_labels.size()), ncases, where, "case labels");
  } else {
    out.x_axis = thread_to_numeric_values(case_thread, where, "case axis");
    require_size(static_cast<std::int64_t>(out.x_axis.size()), ncases, where, "case axis");
  }

  auto depth_axis = build_depth_axis(depth_labels, positions, where);
  out.y_axis = std::move(depth_axis.values);
  out.y_labels = std::move(depth_axis.labels);

  std::vector<double> z_values;
  z_values.reserve(static_cast<std::size_t>(depth * ncases));
  for (const auto p : positions) {
    const auto& frame = frames[static_cast<std::size_t>(p)];
    const auto values = thread_to_numeric_values(frame.features(feature), where, feature);
    require_size(static_cast<std::int64_t>(values.size()), ncases, where, feature);
    z_values.insert(z_values.end(), values.begin(), values.end());
  }
  out.z = pluss::make_series(z_values, {depth, ncases});
  return out;
}

std::vector<double> rolling_mean(std::span<const double> values, std::int64_t window) {
  const auto w = std::max<std::int64_t>(1, window);
  std::vector<double> out(values.size(), std::numeric_limits<double>::quiet_NaN());
  double sum = 0.0;
  std::int64_t count = 0;
  for (const auto i : std::views::iota(std::size_t{0}, values.size())) {
    const auto v = values[i];
    if (std::isfinite(v)) {
      sum += v;
      ++count;
    }
    if (i >= static_cast<std::size_t>(w)) {
      const auto stale = values[i - static_cast<std::size_t>(w)];
      if (std::isfinite(stale)) {
        sum -= stale;
        --count;
      }
    }
    if (count > 0) {
      out[i] = sum / static_cast<double>(count);
    }
  }
  return out;
}

struct ResolvedFrameAxis {
  std::vector<double> values{};
  std::vector<std::string> labels{};
  std::string label{"case"};
};

ResolvedFrameAxis resolve_frame_axis(const DataFrame& frame,
                                     std::optional<std::string_view> x_feature,
                                     std::string_view where) {
  const auto& axis_thread = x_feature.has_value() ? frame.features(*x_feature) : frame.cases();
  const auto axis_label = x_feature.has_value() ? std::string(*x_feature) : std::string("case");
  ResolvedFrameAxis axis;
  axis.label = axis_label;
  if (thread_prefers_labels(axis_thread)) {
    axis.values = positional_axis(frame.ncases());
    axis.labels = thread_to_labels(axis_thread);
    require_size(static_cast<std::int64_t>(axis.labels.size()), frame.ncases(), where, "x labels");
    return axis;
  }
  axis.values = thread_to_numeric_values(axis_thread, where, axis_label);
  require_size(static_cast<std::int64_t>(axis.values.size()), frame.ncases(), where, "x axis");
  return axis;
}

} // namespace

AtlasPlotAccessor::AtlasPlotAccessor(std::vector<DataFrame> frames,
                                     std::optional<Thread> depth_labels,
                                     AtlasPlotOptions options)
    : frames_(std::move(frames)), depth_labels_(std::move(depth_labels)), options_(std::move(options)) {}

DataFramePlotAccessor AtlasPlotAccessor::frames(std::int64_t frame_index) const {
  const auto idx = normalize_frame_index(frame_index, static_cast<std::int64_t>(frames_.size()));
  return frames_[static_cast<std::size_t>(idx)].plot(options_.figure);
}

pluss::pl::Studio AtlasPlotAccessor::frames(pluss::msharp::RangeView::Int frame_indices,
                                            std::string_view y_feature,
                                            std::optional<std::string_view> x_feature) const {
  return frames(frame_indices, y_feature, x_feature, options_.frames);
}

pluss::pl::Studio AtlasPlotAccessor::frames(pluss::msharp::RangeView::Int frame_indices,
                                            std::string_view y_feature,
                                            std::optional<std::string_view> x_feature,
                                            const AtlasFramesPlotOptions& options) const {
  PLUSS_CHECK(!frames_.empty(), "Atlas::plot().frames: atlas has no frames");
  const auto selected = normalize_frame_indices(frame_indices, static_cast<std::int64_t>(frames_.size()));
  const auto labels = depth_legend_labels(depth_labels_, selected, "Atlas::plot().frames");

  std::vector<ResolvedFrameAxis> axes;
  std::vector<std::vector<double>> y_values;
  std::vector<pluss::Series> xs_series;
  std::vector<pluss::Series> ys_series;
  axes.reserve(selected.size());
  y_values.reserve(selected.size());
  xs_series.reserve(selected.size());
  ys_series.reserve(selected.size());

  for (const auto p : selected) {
    const auto& frame = frames_[static_cast<std::size_t>(p)];
    PLUSS_CHECK(frame.has_feature(y_feature),
                "Atlas::plot().frames: missing y feature '" + std::string(y_feature) + "' in one or more frames");
    auto axis = resolve_frame_axis(frame, x_feature, "Atlas::plot().frames");
    auto values = thread_to_numeric_values(frame.features(y_feature), "Atlas::plot().frames", y_feature);
    require_size(static_cast<std::int64_t>(values.size()), frame.ncases(), "Atlas::plot().frames", y_feature);
    xs_series.push_back(pluss::make_series(axis.values));
    ys_series.push_back(pluss::make_series(values));
    axes.push_back(std::move(axis));
    y_values.push_back(std::move(values));
  }

  auto fig = make_studio();
  if (options.mode == AtlasFrameSelectionMode::Facet) {
    flash::RelGridPlotOptions facet_options;
    facet_options.kind = flash::RelGridKind::Line;
    facet_options.rows = options.rows;
    facet_options.cols = options.cols;
    facet_options.grid_gap_x = options.grid_gap_x;
    facet_options.grid_gap_y = options.grid_gap_y;
    facet_options.show_grid = options.show_grid;
    facet_options.titles = labels;
    facet_options.y_labels.assign(labels.size(), std::string(y_feature));
    facet_options.x_labels.assign(labels.size(), axes.front().label);
    flash::relgridplot(fig, xs_series, ys_series, facet_options);
    apply_figure_options(fig);
    return fig;
  }

  auto& pane = fig.pane2("atlas_frames_overlay");
  pane.grid(options.show_grid ? pluss::pl::GridStyle::Major : pluss::pl::GridStyle::None)
      .legend(pluss::pl::KeyPlace::Auto)
      .x_label(axes.front().label)
      .y_label(std::string(y_feature));
  if (!axes.front().labels.empty()) {
    const auto axis_label_views = label_views(axes.front().labels);
    pane.x_ticks(axes.front().values, pluss::msharp::RangeView::Of<std::string_view>(axis_label_views));
  }
  for (const auto i : std::views::iota(std::size_t{0}, y_values.size())) {
    flash::lineplot(pane, axes[i].values, y_values[i])
        .label(labels[i])
        .stroke(color_for_index(i))
        .width(std::max(0.5F, options.line_width));
  }
  apply_figure_options(fig);
  return fig;
}

void AtlasPlotAccessor::apply_figure_options(pluss::pl::Studio& figure) const {
  const double width = options_.figure.width.value_or(figure.extent().width);
  const double height = options_.figure.height.value_or(figure.extent().height);
  figure.size(width, height);
  if (options_.figure.use_tight_layout) {
    figure.tight_layout(true, options_.figure.tight_layout_padding);
  }
}

pluss::pl::Studio AtlasPlotAccessor::surface(std::string_view feature) const {
  return surface(feature, options_.surface);
}

pluss::pl::Studio AtlasPlotAccessor::surface(std::string_view feature,
                                             const Surface3DPlotOptions& options) const {
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  auto grid = build_depth_case_grid(frames_, depth_labels_, selected_depth, feature, "Atlas::plot().surface");

  auto fig = make_studio();
  auto& pane = fig.pane3("atlas_surface");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label("case").y_label("depth").z_label(std::string(feature));
  if (!grid.x_labels.empty()) {
    const auto x_label_views = label_views(grid.x_labels);
    pane.x_ticks(grid.x_axis, pluss::msharp::RangeView::Of<std::string_view>(x_label_views));
  }
  if (!grid.y_labels.empty()) {
    const auto y_label_views = label_views(grid.y_labels);
    pane.y_ticks(grid.y_axis, pluss::msharp::RangeView::Of<std::string_view>(y_label_views));
  }
  plot::detail::apply_3d_scene_options(pane, options.scene);
  auto& trace = flash::sheet(pane, grid.x_axis, grid.y_axis, grid.z).label(std::string(feature));
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::wireframe(std::string_view feature) const {
  return wireframe(feature, options_.wireframe);
}

pluss::pl::Studio AtlasPlotAccessor::wireframe(std::string_view feature,
                                               const Wireframe3DPlotOptions& options) const {
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  auto grid = build_depth_case_grid(frames_, depth_labels_, selected_depth, feature, "Atlas::plot().wireframe");

  auto fig = make_studio();
  auto& pane = fig.pane3("atlas_wireframe");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label("case").y_label("depth").z_label(std::string(feature));
  if (!grid.x_labels.empty()) {
    const auto x_label_views = label_views(grid.x_labels);
    pane.x_ticks(grid.x_axis, pluss::msharp::RangeView::Of<std::string_view>(x_label_views));
  }
  if (!grid.y_labels.empty()) {
    const auto y_label_views = label_views(grid.y_labels);
    pane.y_ticks(grid.y_axis, pluss::msharp::RangeView::Of<std::string_view>(y_label_views));
  }
  plot::detail::apply_3d_scene_options(pane, options.scene);
  auto& trace = flash::lattice(pane, grid.x_axis, grid.y_axis, grid.z).label(std::string(feature));
  plot::detail::apply_3d_trace_options(trace, options.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::cloud3(std::string_view x_feature,
                                            std::string_view y_feature,
                                            std::string_view z_feature) const {
  return cloud3(x_feature, y_feature, z_feature, options_.cloud3);
}

pluss::pl::Studio AtlasPlotAccessor::cloud3(std::string_view x_feature,
                                            std::string_view y_feature,
                                            std::string_view z_feature,
                                            const Scatter3DPlotOptions& options) const {
  PLUSS_CHECK(!frames_.empty(), "Atlas::plot().cloud3: atlas has no frames");
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  const auto depth = static_cast<std::int64_t>(selected_depth.size());
  const auto ncases = frames_[static_cast<std::size_t>(selected_depth.front())].ncases();
  PLUSS_CHECK(ncases > 0, "Atlas::plot().cloud3: frame has zero cases");

  const auto labels = depth_legend_labels(depth_labels_, selected_depth, "Atlas::plot().cloud3");

  auto fig = make_studio();
  auto& pane = fig.pane3("atlas_cloud3");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);

  for (const auto i : std::views::iota(std::size_t{0}, selected_depth.size())) {
    const auto& frame = frames_[static_cast<std::size_t>(selected_depth[i])];
    require_size(frame.ncases(), ncases, "Atlas::plot().cloud3", "case axis");
    PLUSS_CHECK(
        frame.has_feature(x_feature) && frame.has_feature(y_feature) && frame.has_feature(z_feature),
        "Atlas::plot().cloud3: one or more requested features are missing in one or more frames");

    const auto x = thread_to_numeric_values(frame.features(x_feature), "Atlas::plot().cloud3", x_feature);
    const auto y = thread_to_numeric_values(frame.features(y_feature), "Atlas::plot().cloud3", y_feature);
    const auto z = thread_to_numeric_values(frame.features(z_feature), "Atlas::plot().cloud3", z_feature);

    require_size(static_cast<std::int64_t>(x.size()), ncases, "Atlas::plot().cloud3", x_feature);
    require_size(static_cast<std::int64_t>(y.size()), ncases, "Atlas::plot().cloud3", y_feature);
    require_size(static_cast<std::int64_t>(z.size()), ncases, "Atlas::plot().cloud3", z_feature);

    auto& trace = flash::cloud3(
                      pane,
                      x,
                      y,
                      z,
                      std::array<double, 1>{std::max(1.0, options.default_marker_size)})
                      .label(labels[i]);
    trace.stroke(color_for_index(i));
    plot::detail::apply_3d_trace_options(trace, options.trace);
  }

  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::depth(std::string_view feature) const {
  return depth(feature, options_.depth);
}

pluss::pl::Studio AtlasPlotAccessor::depth(std::string_view feature, const AtlasDepthPlotOptions& options) const {
  PLUSS_CHECK(!frames_.empty(), "Atlas::plot().depth: atlas has no frames");
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  const auto depth_size = static_cast<std::int64_t>(selected_depth.size());
  const auto ncases = frames_[static_cast<std::size_t>(selected_depth.front())].ncases();
  PLUSS_CHECK(ncases > 0, "Atlas::plot().depth: frame has zero cases");

  auto depth_axis = build_depth_axis(depth_labels_, selected_depth, "Atlas::plot().depth");
  std::vector<double> center_values;
  center_values.reserve(static_cast<std::size_t>(depth_size));

  const bool use_lower = options.lower.has_value();
  const bool use_upper = options.upper.has_value();
  std::vector<double> lower_values;
  std::vector<double> upper_values;
  if (use_lower) {
    lower_values.reserve(static_cast<std::size_t>(depth_size));
  }
  if (use_upper) {
    upper_values.reserve(static_cast<std::size_t>(depth_size));
  }

  for (const auto p : selected_depth) {
    const auto& frame = frames_[static_cast<std::size_t>(p)];
    require_size(frame.ncases(), ncases, "Atlas::plot().depth", "case axis");
    PLUSS_CHECK(
        frame.has_feature(feature),
        std::string("Atlas::plot().depth: missing feature '") + std::string(feature) + "'");
    const auto values = thread_to_numeric_values(frame.features(feature), "Atlas::plot().depth", feature);
    center_values.push_back(reduce_statistic(values, options.center));
    if (use_lower) {
      lower_values.push_back(reduce_statistic(values, *options.lower));
    }
    if (use_upper) {
      upper_values.push_back(reduce_statistic(values, *options.upper));
    }
  }

  auto fig = make_studio();
  auto& pane = fig.pane2("atlas_depth");
  pane.grid(options.show_grid ? pluss::pl::GridStyle::Major : pluss::pl::GridStyle::None)
      .legend(pluss::pl::KeyPlace::Auto)
      .x_label("depth")
      .y_label(std::string(feature));
  if (!depth_axis.labels.empty()) {
    const auto depth_label_views = label_views(depth_axis.labels);
    pane.x_ticks(depth_axis.values, pluss::msharp::RangeView::Of<std::string_view>(depth_label_views));
  }

  if (use_lower) {
    flash::lineplot(pane, depth_axis.values, lower_values)
        .label("lower")
        .stroke(pluss::pl::Rgba::from_u8(148, 163, 184))
        .width(std::max(0.5F, options.band_trace_width));
  }
  if (use_upper) {
    flash::lineplot(pane, depth_axis.values, upper_values)
        .label("upper")
        .stroke(pluss::pl::Rgba::from_u8(148, 163, 184))
        .width(std::max(0.5F, options.band_trace_width));
  }
  flash::lineplot(pane, depth_axis.values, center_values)
      .label("center")
      .stroke(pluss::pl::Rgba::from_u8(15, 118, 110))
      .width(std::max(0.5F, options.center_trace_width));

  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::depth_compare(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return depth_compare(features, options_.depth_compare);
}

pluss::pl::Studio AtlasPlotAccessor::depth_compare(pluss::msharp::RangeView::Of<std::string_view> features,
                                                   const AtlasDepthComparePlotOptions& options) const {
  const auto selected = resolve_depth_features(
      features, frames_, options.max_features, "Atlas::plot().depth_compare");
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, "Atlas::plot().depth_compare");

  auto fig = make_studio();
  auto& pane = fig.pane2("atlas_depth_compare");
  pane.grid(options.show_grid ? pluss::pl::GridStyle::Major : pluss::pl::GridStyle::None)
      .legend(pluss::pl::KeyPlace::Auto)
      .x_label("depth")
      .y_label("value");
  if (!depth_axis.labels.empty()) {
    const auto depth_label_views = label_views(depth_axis.labels);
    pane.x_ticks(depth_axis.values, pluss::msharp::RangeView::Of<std::string_view>(depth_label_views));
  }

  for (const auto feature_index : std::views::iota(std::size_t{0}, selected.size())) {
    const auto& feature = selected[feature_index];
    const auto values = depth_statistic_values(
        frames_, selected_depth, feature, options.statistic, "Atlas::plot().depth_compare");
    flash::lineplot(pane, depth_axis.values, values)
        .label(feature)
        .stroke(color_for_index(feature_index))
        .width(std::max(0.5F, options.line_width));
  }

  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::depth_facet(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return depth_facet(features, options_.depth_facet);
}

pluss::pl::Studio AtlasPlotAccessor::depth_facet(pluss::msharp::RangeView::Of<std::string_view> features,
                                                 const AtlasDepthFacetPlotOptions& options) const {
  const auto selected = resolve_depth_features(
      features, frames_, options.max_features, "Atlas::plot().depth_facet");
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, "Atlas::plot().depth_facet");

  std::vector<pluss::Series> xs;
  std::vector<pluss::Series> ys;
  std::vector<std::string> titles;
  std::vector<std::string> x_labels;
  std::vector<std::string> y_labels;
  xs.reserve(selected.size());
  ys.reserve(selected.size());
  titles.reserve(selected.size());
  x_labels.reserve(selected.size());
  y_labels.reserve(selected.size());

  const auto x_series = pluss::make_series(depth_axis.values);
  for (const auto& feature : selected) {
    auto values = depth_statistic_values(
        frames_, selected_depth, feature, options.statistic, "Atlas::plot().depth_facet");
    xs.push_back(x_series);
    ys.push_back(pluss::make_series(values));
    titles.push_back(feature);
    x_labels.push_back("depth");
    y_labels.push_back(feature);
  }

  auto fig = make_studio();
  flash::RelGridPlotOptions rel_options;
  rel_options.kind = flash::RelGridKind::Line;
  rel_options.rows = options.rows;
  rel_options.cols = options.cols;
  rel_options.grid_gap_x = options.grid_gap_x;
  rel_options.grid_gap_y = options.grid_gap_y;
  rel_options.show_grid = options.show_grid;
  rel_options.titles = std::move(titles);
  rel_options.x_labels = std::move(x_labels);
  rel_options.y_labels = std::move(y_labels);
  flash::relgridplot(fig, xs, ys, rel_options);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::depth_rolling(std::string_view feature) const {
  return depth_rolling(feature, options_.depth_rolling);
}

pluss::pl::Studio AtlasPlotAccessor::depth_rolling(std::string_view feature,
                                                   const AtlasDepthRollingPlotOptions& options) const {
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, "Atlas::plot().depth_rolling");
  const auto raw_values = depth_statistic_values(
      frames_,
      selected_depth,
      feature,
      options.statistic,
      "Atlas::plot().depth_rolling");
  const auto rolling_values = rolling_mean(raw_values, options.window);

  auto fig = make_studio();
  auto& pane = fig.pane2("atlas_depth_rolling");
  pane.grid(options.show_grid ? pluss::pl::GridStyle::Major : pluss::pl::GridStyle::None)
      .legend(pluss::pl::KeyPlace::Auto)
      .x_label("depth")
      .y_label(std::string(feature));
  if (!depth_axis.labels.empty()) {
    const auto depth_label_views = label_views(depth_axis.labels);
    pane.x_ticks(depth_axis.values, pluss::msharp::RangeView::Of<std::string_view>(depth_label_views));
  }
  if (options.show_raw) {
    flash::lineplot(pane, depth_axis.values, raw_values)
        .label("raw")
        .stroke(pluss::pl::Rgba::from_u8(148, 163, 184))
        .width(std::max(0.5F, options.raw_line_width));
  }
  flash::lineplot(pane, depth_axis.values, rolling_values)
      .label("rolling")
      .stroke(pluss::pl::Rgba::from_u8(15, 118, 110))
      .width(std::max(0.5F, options.rolling_line_width));
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::triplot() const {
  return triplot({}, options_.triplot);
}

pluss::pl::Studio AtlasPlotAccessor::triplot(const pluss::pl::flash::TriplotOptions& options) const {
  return triplot({}, options);
}

pluss::pl::Studio AtlasPlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return triplot(features, options_.triplot);
}

pluss::pl::Studio AtlasPlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> features,
                                             const pluss::pl::flash::TriplotOptions& options) const {
  const auto selected = resolve_triplet_features(features, frames_);
  auto columns = flatten_feature_columns(frames_, selected, "Atlas::plot().triplot");

  auto fig = make_studio();
  auto tri_options = options;
  if (tri_options.labels.empty()) {
    tri_options.labels = selected;
  }
  flash::triplot(fig, columns, tri_options);
  apply_figure_options(fig);
  return fig;
}

} // namespace pluss::table
