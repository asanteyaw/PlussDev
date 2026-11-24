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

std::int64_t normalize_frame_index(std::int64_t idx, std::int64_t depth, const AtlasPlotWords& w = {}) {
  PLUSS_CHECK(depth > 0, w.where + ".frames: " + std::string(w.container) + " has no " + std::string(w.slices));
  std::int64_t out = idx;
  if (out < 0) {
    out += depth;
  }
  PLUSS_CHECK(out >= 0 && out < depth, w.where + ".frames: " + std::string(w.slice) + " index out of bounds");
  return out;
}

std::vector<std::int64_t> normalize_frame_indices(pluss::msharp::RangeView::Int indices,
                                                  std::int64_t depth,
                                                  const AtlasPlotWords& w = {}) {
  PLUSS_CHECK(indices.size() != 0, w.where + ".frames: at least one " + std::string(w.slice) + " index is required");
  std::vector<std::int64_t> out;
  out.reserve(indices.size());
  std::vector<std::uint8_t> seen(static_cast<std::size_t>(depth), static_cast<std::uint8_t>(0));
  for (const auto idx : indices) {
    const auto normalized = normalize_frame_index(idx, depth, w);
    auto& visited = seen[static_cast<std::size_t>(normalized)];
    if (visited == static_cast<std::uint8_t>(0)) {
      visited = static_cast<std::uint8_t>(1);
      out.push_back(normalized);
    }
  }
  PLUSS_CHECK(!out.empty(), w.where + ".frames: no valid " + std::string(w.slice) + " indices selected");
  return out;
}

std::vector<std::int64_t> depth_positions(std::int64_t depth,
                                          const AtlasDepthAxisOptions& options,
                                          const AtlasPlotWords& w = {}) {
  PLUSS_CHECK(depth > 0, w.where + ": " + std::string(w.container) + " has no " + std::string(w.slices));
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

bool shelf_prefers_labels(const Shelf& t) {
  return t.kind() == ShelfKind::Note || t.kind() == ShelfKind::Timeline;
}

std::vector<std::string> shelf_to_labels(const Shelf& shelf) {
  const Shelf note_like = (shelf.kind() == ShelfKind::Note) ? shelf : shelf.cast().note();
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

pluss::Series shelf_to_numeric_series(const Shelf& t, std::string_view where, std::string_view feature_name) {
  Shelf numeric_shelf = t;
  switch (t.kind()) {
    case ShelfKind::Series:
      break;
    case ShelfKind::Timeline:
      numeric_shelf = Shelf(t.as_timeline().epoch_nanos(), detail::validity_mask(t));
      break;
    case ShelfKind::Note:
      numeric_shelf = t.cast({.series_dtype = pluss::codem::ScalarType::Float64}).series();
      break;
    default:
      PLUSS_FAIL(std::string(where) + ": feature '" + std::string(feature_name) + "' is not numeric");
  }

  auto values = numeric_shelf.as_series().to(pluss::codem::ScalarType::Float64);
  if (detail::has_mask(numeric_shelf)) {
    const auto nan_fill = pluss::series::full(
        {values.numel()},
        std::numeric_limits<double>::quiet_NaN(),
        pluss::codem::ScalarType::Float64);
    values = pluss::series::where(detail::validity_mask(numeric_shelf)->logical_not(), nan_fill, values);
  }
  return values.reshape({values.numel()});
}

std::vector<double> shelf_to_numeric_values(const Shelf& t,
                                             std::string_view where,
                                             std::string_view feature_name) {
  return shelf_to_numeric_series(t, where, feature_name).host<double>();
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

std::vector<std::string> depth_legend_labels(const std::optional<Shelf>& depth_labels,
                                             std::span<const std::int64_t> positions,
                                             std::string_view where,
                                             std::string_view slice = "frame") {
  const auto depth = static_cast<std::int64_t>(positions.size());
  PLUSS_CHECK(depth > 0, std::string(where) + ": empty depth selection");
  if (!depth_labels.has_value()) {
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(depth));
    for (const auto p : positions) {
      labels.push_back(std::string(slice) + " " + std::to_string(p));
    }
    return labels;
  }

  if (shelf_prefers_labels(*depth_labels)) {
    const auto source = shelf_to_labels(*depth_labels);
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

  const auto values = shelf_to_numeric_values(*depth_labels, where, "depth axis");
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
  pluss::Series z = pluss::series::undefined();
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

DepthAxis2D build_depth_axis(const std::optional<Shelf>& depth_labels,
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
  if (shelf_prefers_labels(*depth_labels)) {
    axis.values = positional_axis(depth);
    const auto source = shelf_to_labels(*depth_labels);
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
  const auto source = shelf_to_numeric_values(*depth_labels, where, "depth axis");
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
  const auto numeric = frames.front().feataxis().series();
  auto names = numeric.features();
  PLUSS_CHECK(
      static_cast<std::int64_t>(names.size()) >= 3,
      "Atlas::plot().triplot: requires at least three numeric series features");
  return names;
}

std::string names_list(const std::vector<std::string>& names) {
  return pluss::pl::flash::detail::column_name_list(names);
}

// A name handed to a depth plot that the stack cannot draw: not there at all (with the closest name and
// the available ones) or there but not numeric. Worded with the container's own noun.
[[noreturn]] void fail_unplottable_name(const std::string& name,
                                        const std::vector<std::string>& all_names,
                                        const std::vector<std::string>& numeric_names,
                                        std::string_view where,
                                        const AtlasPlotWords& w) {
  const std::string noun(w.selection.noun);
  const std::string nouns(w.selection.nouns);
  if (std::find(all_names.begin(), all_names.end(), name) == all_names.end()) {
    PLUSS_FAIL(std::string(where) + ": unknown " + noun + " '" + name + "'." +
               pluss::pl::flash::detail::closest_name_hint(name, all_names) + " Available " + nouns + " = " +
               names_list(all_names) + ". Fix the spelling or remove '" + name + "'.");
  }
  PLUSS_FAIL(std::string(where) + ": " + noun + " '" + name + "' is not numeric; only numeric " + nouns +
             " can be drawn here. Numeric " + nouns + " = " + names_list(numeric_names) + ". Remove '" + name +
             "' or convert it to a numeric " + noun + " first.");
}

void require_slices(const std::vector<DataFrame>& frames, std::string_view where, const AtlasPlotWords& w) {
  PLUSS_CHECK(!frames.empty(), std::string(where) + ": " + std::string(w.container) + " has no " + std::string(w.slices));
}

std::vector<std::string> resolve_name_selection(const FeatureSelection& selection,
                                                const std::vector<DataFrame>& frames,
                                                std::string_view where,
                                                const AtlasPlotWords& w) {
  const auto numeric_names = frames.front().feataxis().series().features();
  return detail::resolve_plot_selection(selection.all_features, selection.include_features,
                                        selection.exclude_features, w.selection, frames.front().features(),
                                        numeric_names, where);
}

std::vector<std::string> resolve_explicit_names(pluss::msharp::RangeView::Of<std::string_view> requested,
                                                const std::vector<DataFrame>& frames,
                                                std::string_view where,
                                                const AtlasPlotWords& w) {
  const auto all_names = frames.front().features();
  const auto numeric_names = frames.front().feataxis().series().features();
  const std::unordered_set<std::string> available_set(numeric_names.begin(), numeric_names.end());
  std::vector<std::string> out;
  out.reserve(requested.size());
  std::unordered_set<std::string> selected_set;
  selected_set.reserve(requested.size());
  for (const auto name_view : requested) {
    const std::string name(name_view);
    if (!selected_set.insert(name).second) {
      continue;
    }
    if (!available_set.contains(name)) fail_unplottable_name(name, all_names, numeric_names, where, w);
    out.push_back(name);
  }
  return out;
}

std::vector<std::string> resolve_triplet_features(pluss::msharp::RangeView::Of<std::string_view> requested,
                                                  const std::vector<DataFrame>& frames,
                                                  const FeatureSelection& selection,
                                                  const AtlasPlotWords& w) {
  const std::string where = w.where + ".triplot";
  require_slices(frames, where, w);
  const std::string nouns(w.selection.nouns);
  std::vector<std::string> names = requested.size() == 0 ? resolve_name_selection(selection, frames, where, w)
                                                         : resolve_explicit_names(requested, frames, where, w);
  PLUSS_CHECK(static_cast<std::int64_t>(names.size()) >= 3,
              where + ": requires at least three numeric " + nouns + "; found " + std::to_string(names.size()) +
                  ". Name at least three " + nouns + " or set " + std::string(w.selection.all) + ".");
  return names;
}

std::vector<std::string> resolve_depth_features(pluss::msharp::RangeView::Of<std::string_view> requested,
                                                const std::vector<DataFrame>& frames,
                                                std::int64_t max_features,
                                                std::string_view where,
                                                const FeatureSelection& selection,
                                                const AtlasPlotWords& w) {
  require_slices(frames, where, w);
  std::vector<std::string> features = requested.size() == 0 ? resolve_name_selection(selection, frames, where, w)
                                                            : resolve_explicit_names(requested, frames, where, w);
  PLUSS_CHECK(!features.empty(), std::string(where) + ": requires at least one numeric " + std::string(w.selection.noun));
  if (max_features > 0 && static_cast<std::int64_t>(features.size()) > max_features) {
    features.resize(static_cast<std::size_t>(max_features));
  }
  return features;
}

std::vector<double> depth_statistic_values(const std::vector<DataFrame>& frames,
                                           std::span<const std::int64_t> positions,
                                           std::string_view feature,
                                           AtlasDepthStatistic statistic,
                                           std::string_view where,
                                           const AtlasPlotWords& w = {}) {
  PLUSS_CHECK(!positions.empty(), std::string(where) + ": empty depth selection");

  std::vector<double> values;
  values.reserve(positions.size());
  for (const auto p : positions) {
    const auto& frame = frames[static_cast<std::size_t>(p)];
    PLUSS_CHECK(frame.ncases() > 0, std::string(where) + ": " + std::string(w.slice) + " has zero " + std::string(w.rows));
    PLUSS_CHECK(
        frame.get(feature).has_value(),
        std::string(where) + ": missing " + std::string(w.selection.noun) + " '" + std::string(feature) + "'");
    const auto frame_values = shelf_to_numeric_values(frame.features(feature), where, feature);
    values.push_back(reduce_statistic(frame_values, statistic));
  }
  return values;
}

std::vector<pluss::Series> flatten_feature_columns(const std::vector<DataFrame>& frames,
                                                   const std::vector<std::string>& features,
                                                   std::string_view where,
                                                   const AtlasPlotWords& w = {}) {
  require_slices(frames, where, w);
  std::int64_t total_cases = 0;
  for (const auto& frame : frames) {
    PLUSS_CHECK(frame.ncases() > 0, std::string(where) + ": " + std::string(w.slice) + " has zero " + std::string(w.rows));
    total_cases += frame.ncases();
  }

  std::vector<std::vector<double>> flattened(features.size());
  for (auto& column : flattened) {
    column.reserve(static_cast<std::size_t>(total_cases));
  }

  for (const auto& frame : frames) {
    for (const auto i : std::views::iota(std::size_t{0}, features.size())) {
      const auto& feature = features[i];
      PLUSS_CHECK(frame.get(feature).has_value(),
                  std::string(where) + ": missing " + std::string(w.selection.noun) + " '" + feature + "'");
      const auto values = shelf_to_numeric_values(frame.features(feature), where, feature);
      require_size(static_cast<std::int64_t>(values.size()), frame.ncases(), where, feature);
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
                                  const std::optional<Shelf>& depth_labels,
                                  std::span<const std::int64_t> positions,
                                  std::string_view feature,
                                  std::string_view where,
                                  const AtlasPlotWords& w = {}) {
  PLUSS_CHECK(!positions.empty(), std::string(where) + ": empty depth selection");

  const auto depth = static_cast<std::int64_t>(positions.size());
  bool all_case_labeled = true;
  std::int64_t max_cases = 0;
  for (const auto p : positions) {
    const auto& frame = frames[static_cast<std::size_t>(p)];
    PLUSS_CHECK(frame.ncases() > 0, std::string(where) + ": " + std::string(w.slice) + " has zero " + std::string(w.rows));
    PLUSS_CHECK(frame.get(feature).has_value(),
                std::string(where) + ": missing " + std::string(w.selection.noun) + " '" + std::string(feature) +
                    "' in one or more " + std::string(w.slices));
    max_cases = std::max(max_cases, frame.ncases());
    all_case_labeled = all_case_labeled && shelf_prefers_labels(frame.labels().values());
  }

  AtlasGrid3D out;
  std::vector<std::vector<double>> rows;
  rows.reserve(static_cast<std::size_t>(depth));

  if (all_case_labeled) {
    std::vector<std::string> union_labels;
    std::unordered_map<std::string, std::int64_t> union_index;
    union_labels.reserve(static_cast<std::size_t>(max_cases));

    for (const auto p : positions) {
      const auto& frame = frames[static_cast<std::size_t>(p)];
      const auto labels = shelf_to_labels(frame.labels().values());
      require_size(static_cast<std::int64_t>(labels.size()), frame.ncases(), where, "case labels");
      for (const auto& label : labels) {
        if (!union_index.contains(label)) {
          const auto pos = static_cast<std::int64_t>(union_labels.size());
          union_index.emplace(label, pos);
          union_labels.push_back(label);
        }
      }
    }

    const auto ncases = static_cast<std::int64_t>(union_labels.size());
    out.x_axis = positional_axis(ncases);
    out.x_labels = union_labels;

    for (const auto p : positions) {
      const auto& frame = frames[static_cast<std::size_t>(p)];
      const auto labels = shelf_to_labels(frame.labels().values());
      const auto values = shelf_to_numeric_values(frame.features(feature), where, feature);
      require_size(static_cast<std::int64_t>(values.size()), frame.ncases(), where, feature);

      std::vector<double> row(static_cast<std::size_t>(ncases), std::numeric_limits<double>::quiet_NaN());
      for (std::size_t i = 0; i < labels.size(); ++i) {
        row[static_cast<std::size_t>(union_index.at(labels[i]))] = values[i];
      }
      rows.push_back(std::move(row));
    }
  } else {
    const auto ncases = max_cases;
    out.x_axis = positional_axis(ncases);

    for (const auto p : positions) {
      const auto& frame = frames[static_cast<std::size_t>(p)];
      const auto values = shelf_to_numeric_values(frame.features(feature), where, feature);
      require_size(static_cast<std::int64_t>(values.size()), frame.ncases(), where, feature);

      std::vector<double> row(static_cast<std::size_t>(ncases), std::numeric_limits<double>::quiet_NaN());
      std::copy(values.begin(), values.end(), row.begin());
      rows.push_back(std::move(row));
    }
  }

  auto depth_axis = build_depth_axis(depth_labels, positions, where);
  out.y_axis = std::move(depth_axis.values);
  out.y_labels = std::move(depth_axis.labels);

  std::vector<double> z_values;
  const auto ncases = static_cast<std::int64_t>(out.x_axis.size());
  z_values.reserve(static_cast<std::size_t>(depth * ncases));
  for (auto& row : rows) {
    z_values.insert(z_values.end(), row.begin(), row.end());
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
  const auto& axis_shelf = x_feature.has_value() ? frame.features(*x_feature) : frame.labels().values();
  const auto axis_label = x_feature.has_value() ? std::string(*x_feature) : std::string("case");
  ResolvedFrameAxis axis;
  axis.label = axis_label;
  if (shelf_prefers_labels(axis_shelf)) {
    axis.values = positional_axis(frame.ncases());
    axis.labels = shelf_to_labels(axis_shelf);
    require_size(static_cast<std::int64_t>(axis.labels.size()), frame.ncases(), where, "x labels");
    return axis;
  }
  axis.values = shelf_to_numeric_values(axis_shelf, where, axis_label);
  require_size(static_cast<std::int64_t>(axis.values.size()), frame.ncases(), where, "x axis");
  return axis;
}

} // namespace

AtlasPlotAccessor::AtlasPlotAccessor(std::vector<DataFrame> frames,
                                     std::optional<Shelf> depth_labels,
                                     AtlasPlotOptions options,
                                     AtlasPlotWords words)
    : frames_(std::move(frames)),
      depth_labels_(std::move(depth_labels)),
      options_(std::move(options)),
      words_(std::move(words)) {}

DataFramePlotAccessor AtlasPlotAccessor::frames(std::int64_t frame_index) const {
  const auto idx = normalize_frame_index(frame_index, static_cast<std::int64_t>(frames_.size()), words_);
  return frames_[static_cast<std::size_t>(idx)].plot(options_.figure);
}

pluss::pl::Studio AtlasPlotAccessor::frames_y(pluss::msharp::RangeView::Int frame_indices,
                                              std::string_view y_feature,
                                              std::optional<std::string_view> x_feature,
                                              const AtlasFramesPlotOptions& options) const {
  require_slices(frames_, words_.where + ".frames", words_);
  const auto selected = normalize_frame_indices(frame_indices, static_cast<std::int64_t>(frames_.size()), words_);
  const auto labels = depth_legend_labels(depth_labels_, selected, words_.where + ".frames", words_.slice);

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
    PLUSS_CHECK(frame.get(y_feature).has_value(),
                words_.where + ".frames: missing y " + std::string(words_.selection.noun) + " '" + std::string(y_feature) + "' in one or more " + std::string(words_.slices));
    auto axis = resolve_frame_axis(frame, x_feature, words_.where + ".frames");
    auto values = shelf_to_numeric_values(frame.features(y_feature), words_.where + ".frames", y_feature);
    require_size(static_cast<std::int64_t>(values.size()), frame.ncases(), words_.where + ".frames", y_feature);
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
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  auto grid = build_depth_case_grid(frames_, depth_labels_, selected_depth, feature, words_.where + ".surface", words_);

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
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  auto grid = build_depth_case_grid(frames_, depth_labels_, selected_depth, feature, words_.where + ".wireframe", words_);

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
  require_slices(frames_, words_.where + ".cloud3", words_);
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  const auto depth = static_cast<std::int64_t>(selected_depth.size());
  (void)depth;

  const auto labels = depth_legend_labels(depth_labels_, selected_depth, words_.where + ".cloud3", words_.slice);

  auto fig = make_studio();
  auto& pane = fig.pane3("atlas_cloud3");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_feature)).y_label(std::string(y_feature)).z_label(std::string(z_feature));
  plot::detail::apply_3d_scene_options(pane, options.scene);

  for (const auto i : std::views::iota(std::size_t{0}, selected_depth.size())) {
    const auto& frame = frames_[static_cast<std::size_t>(selected_depth[i])];
    PLUSS_CHECK(frame.ncases() > 0, words_.where + ".cloud3: " + std::string(words_.slice) + " has zero " + std::string(words_.rows));
    PLUSS_CHECK(
        frame.get(x_feature).has_value() && frame.get(y_feature).has_value() && frame.get(z_feature).has_value(),
        words_.where + ".cloud3: one or more requested " + std::string(words_.selection.nouns) + " are missing in one or more " + std::string(words_.slices));

    const auto x = shelf_to_numeric_values(frame.features(x_feature), words_.where + ".cloud3", x_feature);
    const auto y = shelf_to_numeric_values(frame.features(y_feature), words_.where + ".cloud3", y_feature);
    const auto z = shelf_to_numeric_values(frame.features(z_feature), words_.where + ".cloud3", z_feature);

    require_size(static_cast<std::int64_t>(x.size()), frame.ncases(), words_.where + ".cloud3", x_feature);
    require_size(static_cast<std::int64_t>(y.size()), frame.ncases(), words_.where + ".cloud3", y_feature);
    require_size(static_cast<std::int64_t>(z.size()), frame.ncases(), words_.where + ".cloud3", z_feature);

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
  require_slices(frames_, words_.where + ".depth", words_);
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  const auto depth_size = static_cast<std::int64_t>(selected_depth.size());

  auto depth_axis = build_depth_axis(depth_labels_, selected_depth, words_.where + ".depth");
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
    PLUSS_CHECK(frame.ncases() > 0, words_.where + ".depth: " + std::string(words_.slice) + " has zero " + std::string(words_.rows));
    PLUSS_CHECK(
        frame.get(feature).has_value(),
        words_.where + ".depth: missing " + std::string(words_.selection.noun) + " '" + std::string(feature) + "'");
    const auto values = shelf_to_numeric_values(frame.features(feature), words_.where + ".depth", feature);
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
      features, frames_, options.max_features, words_.where + ".depth_compare",
      options.features.any() ? options.features : options_.figure.selection(), words_);
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, words_.where + ".depth_compare");

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
        frames_, selected_depth, feature, options.statistic, words_.where + ".depth_compare", words_);
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
      features, frames_, options.max_features, words_.where + ".depth_facet",
      options.features.any() ? options.features : options_.figure.selection(), words_);
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, words_.where + ".depth_facet");

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
        frames_, selected_depth, feature, options.statistic, words_.where + ".depth_facet", words_);
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
  const auto selected_depth = depth_positions(static_cast<std::int64_t>(frames_.size()), options_.depth_axis, words_);
  const auto depth_axis = build_depth_axis(depth_labels_, selected_depth, words_.where + ".depth_rolling");
  const auto raw_values = depth_statistic_values(
      frames_,
      selected_depth,
      feature,
      options.statistic,
      words_.where + ".depth_rolling",
      words_);
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

pluss::pl::Studio AtlasPlotAccessor::triplot(const FeatureSelection& columns) const {
  return triplot(columns, options_.triplot);
}

pluss::pl::Studio AtlasPlotAccessor::triplot(const FeatureSelection& columns,
                                             const pluss::pl::flash::TriplotOptions& options) const {
  const auto selected = resolve_triplet_features({}, frames_, columns.any() ? columns : options_.figure.selection(), words_);
  auto flat = flatten_feature_columns(frames_, selected, words_.where + ".triplot", words_);
  auto fig = make_studio();
  auto tri_options = options;
  if (tri_options.labels.empty()) {
    tri_options.labels = selected;
  }
  flash::triplot(fig, flat, tri_options);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio AtlasPlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return triplot(features, options_.triplot);
}

pluss::pl::Studio AtlasPlotAccessor::triplot(pluss::msharp::RangeView::Of<std::string_view> features,
                                             const pluss::pl::flash::TriplotOptions& options) const {
  const auto selected = resolve_triplet_features(features, frames_, options_.figure.selection(), words_);
  auto columns = flatten_feature_columns(frames_, selected, words_.where + ".triplot", words_);

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
