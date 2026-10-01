#pragma once

#include "table/DataFrame.h"
#include "table/Atlas.h"

#include "pluss/codeh/codeh.h"
#include "pluss/pl/flash.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::table {

using FlashRectifyPolicy = ::pluss::pl::flash::FramePolicy;
using FlashFromTableOptions = ::pluss::pl::flash::FromFrameOptions;
using FlashToFrameOptions = ::pluss::pl::flash::ToFrameOptions;
using FlashTableColumns = ::pluss::pl::flash::TableColumns;

namespace detail {

[[nodiscard]] inline bool series_mask_has_missing_(const pluss::Series& mask) {
  const auto flags = mask.to(Platform::CPU).to_vec<bool>();
  return std::any_of(flags.begin(), flags.end(), [](bool v) { return !v; });
}

[[nodiscard]] inline std::optional<pluss::Series> compact_validity_mask_(const Thread& values) {
  const auto valid = values.notnull().as_series();
  return series_mask_has_missing_(valid) ? std::optional<pluss::Series>(valid) : std::nullopt;
}

[[nodiscard]] inline pluss::Series bool_series_from_flags_(
    const std::vector<bool>& flags,
    Platform platform = Platform::CPU) {
  const auto n = static_cast<std::int64_t>(flags.size());
  auto out = pluss::Series::zeros({n}, ScalarType::Bool, platform);
  auto host = out.to(Platform::CPU);
  auto* ptr = reinterpret_cast<bool*>(host.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    ptr[i] = flags[static_cast<std::size_t>(i)];
  }
  if (platform != Platform::CPU) {
    out = host.to(platform);
  } else {
    out = std::move(host);
  }
  return out;
}

[[nodiscard]] inline std::optional<pluss::Series> maybe_padded_validity_(
    const std::optional<pluss::Series>& mask,
    std::int64_t current_size,
    std::int64_t target_size,
    Platform platform) {
  PLUSS_CHECK(target_size >= current_size,
              "flash::Dataset::frame: target size cannot be smaller than source size");
  if (target_size == current_size && !mask.has_value()) {
    return std::nullopt;
  }

  std::vector<bool> flags(static_cast<std::size_t>(target_size), false);
  if (mask.has_value()) {
    const auto host = mask->to(Platform::CPU).to_vec<bool>();
    PLUSS_CHECK(static_cast<std::int64_t>(host.size()) == current_size,
                "flash::Dataset::frame: validity mask length mismatch");
    std::copy(host.begin(), host.end(), flags.begin());
  } else {
    std::fill(flags.begin(), flags.begin() + current_size, true);
  }
  return bool_series_from_flags_(flags, platform);
}

[[nodiscard]] inline Thread rectangularize_series_(
    const pluss::Series& values,
    const std::optional<pluss::Series>& mask,
    std::int64_t target_size,
    FlashRectifyPolicy policy) {
  const auto current_size = values.numel();
  if (current_size == target_size) {
    return mask.has_value() ? Thread(values, *mask) : Thread(values);
  }
  PLUSS_CHECK(policy == FlashRectifyPolicy::PadNull,
              "flash::Dataset::frame: numeric feature is ragged; use FramePolicy::PadNull to rectangularize");
  PLUSS_CHECK(current_size < target_size,
              "flash::Dataset::frame: numeric feature is longer than the selected rectangular size");
  const auto pad = target_size - current_size;
  const auto tail = pluss::Series::full({pad}, 0.0, values.scalar_type(), values.platform());
  const auto padded = values.unite({tail}, 0, true);
  auto padded_mask = maybe_padded_validity_(mask, current_size, target_size, values.platform());
  return padded_mask.has_value() ? Thread(padded, *padded_mask) : Thread(padded);
}

[[nodiscard]] inline Thread rectangularize_note_(
    const pluss::Note& values,
    const std::optional<pluss::Series>& mask,
    std::int64_t target_size,
    FlashRectifyPolicy policy) {
  const auto current_size = values.numel();
  if (current_size == target_size) {
    return mask.has_value() ? Thread(values, *mask) : Thread(values);
  }
  PLUSS_CHECK(policy == FlashRectifyPolicy::PadNull,
              "flash::Dataset::frame: text feature is ragged; use FramePolicy::PadNull to rectangularize");
  PLUSS_CHECK(current_size < target_size,
              "flash::Dataset::frame: text feature is longer than the selected rectangular size");
  std::vector<std::string> padded(static_cast<std::size_t>(target_size));
  for (std::int64_t i = 0; i < current_size; ++i) {
    padded[static_cast<std::size_t>(i)] = values.at(i);
  }
  auto padded_mask = maybe_padded_validity_(mask, current_size, target_size, Platform::CPU);
  auto out = pluss::make_note(std::move(padded));
  return padded_mask.has_value() ? Thread(out, *padded_mask) : Thread(out);
}

[[nodiscard]] inline Thread rectangularize_timeline_(
    const pluss::Timeline& values,
    const std::optional<pluss::Series>& mask,
    std::int64_t target_size,
    FlashRectifyPolicy policy) {
  const auto current_size = values.numel();
  if (current_size == target_size) {
    return mask.has_value() ? Thread(values, *mask) : Thread(values);
  }
  PLUSS_CHECK(policy == FlashRectifyPolicy::PadNull,
              "flash::Dataset::frame: temporal feature is ragged; use FramePolicy::PadNull to rectangularize");
  PLUSS_CHECK(current_size < target_size,
              "flash::Dataset::frame: temporal feature is longer than the selected rectangular size");
  auto ticks = values.epoch_nanos().to(Platform::CPU).to_vec<std::int64_t>();
  ticks.resize(static_cast<std::size_t>(target_size), pluss::Timeline::kNaT);
  const std::array<std::int64_t, 1> shape{target_size};
  auto out = pluss::codeh::ops_internal::timeline_from_epoch_nanos(ticks, shape);
  auto padded_mask = maybe_padded_validity_(mask, current_size, target_size, out.epoch_nanos().platform());
  return padded_mask.has_value() ? Thread(out, *padded_mask) : Thread(out);
}

[[nodiscard]] inline std::vector<std::pair<std::string, Thread>> ordered_flash_columns_(
    const pluss::pl::flash::Dataset& data,
    const FlashToFrameOptions& options) {
  std::vector<std::pair<std::string, Thread>> columns;
  std::vector<std::pair<std::string, std::string>> ordered;

  if (data.feature_order.has_value() && data.feature_kinds.has_value() &&
      data.feature_order->numel() == data.feature_kinds->numel()) {
    ordered.reserve(static_cast<std::size_t>(data.feature_order->numel()));
    for (std::int64_t i = 0; i < data.feature_order->numel(); ++i) {
      ordered.emplace_back(std::string(data.feature_order->at(i)),
                           std::string(data.feature_kinds->at(i)));
    }
  } else {
    auto append = [&](const std::optional<pluss::Note>& names, std::string_view kind) {
      if (!names.has_value()) {
        return;
      }
      for (std::int64_t i = 0; i < names->numel(); ++i) {
        ordered.emplace_back(std::string(names->at(i)), std::string(kind));
      }
    };
    append(data.numeric_names, "numeric");
    append(data.text_names, "text");
    append(data.temporal_names, "temporal");
  }

  std::int64_t target_size = 0;
  bool seen_any = false;
  auto fold_size = [&](std::int64_t size) {
    if (!seen_any) {
      target_size = size;
      seen_any = true;
      return;
    }
    if (options.policy == FlashRectifyPolicy::Strict) {
      PLUSS_CHECK(size == target_size,
                  "flash::Dataset::frame: dataset is ragged; use FramePolicy::PadNull to rectangularize");
    } else {
      target_size = std::max(target_size, size);
    }
  };

  for (const auto& [name, kind] : ordered) {
    if (kind == "numeric") {
      const auto values = data.numeric(name);
      PLUSS_CHECK(values.has_value(), "flash::Dataset::frame: missing numeric feature '" + name + "'");
      fold_size(values->numel());
      continue;
    }
    if (kind == "text" || kind == "note") {
      const auto values = data.text(name);
      PLUSS_CHECK(values.has_value(), "flash::Dataset::frame: missing text feature '" + name + "'");
      fold_size(values->numel());
      continue;
    }
    if (kind == "temporal") {
      const auto values = data.temporal(name);
      PLUSS_CHECK(values.has_value(), "flash::Dataset::frame: missing temporal feature '" + name + "'");
      fold_size(values->numel());
      continue;
    }
    PLUSS_FAIL("flash::Dataset::frame: unsupported feature kind '" + kind + "'");
  }

  columns.reserve(ordered.size());
  for (const auto& [name, kind] : ordered) {
    if (kind == "numeric") {
      columns.emplace_back(name,
                           rectangularize_series_(*data.numeric(name),
                                                  data.numeric_validity_mask(name),
                                                  target_size,
                                                  options.policy));
      continue;
    }
    if (kind == "text" || kind == "note") {
      columns.emplace_back(name,
                           rectangularize_note_(*data.text(name),
                                                data.text_validity_mask(name),
                                                target_size,
                                                options.policy));
      continue;
    }
    columns.emplace_back(name,
                         rectangularize_timeline_(*data.temporal(name),
                                                  data.temporal_validity_mask(name),
                                                  target_size,
                                                  options.policy));
  }

  return columns;
}

[[nodiscard]] inline std::string feature_list_(const std::vector<std::string>& names) {
  std::ostringstream oss;
  oss << "[";
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << names[i];
  }
  oss << "]";
  return oss.str();
}

[[nodiscard]] inline DataFrame single_frame_from_atlas_(
    const Atlas& atlas,
    std::string_view where) {
  PLUSS_CHECK(
      atlas.depth() == 1,
      std::string(where) +
          ": Atlas input requires depth == 1 for direct flash plotting; select a single frame or use Atlas plotting instead");
  return atlas.squeeze();
}

[[nodiscard]] inline Thread require_named_feature_(
    const DataFrame& frame,
    std::string_view feature_name,
    std::string_view where) {
  PLUSS_CHECK(frame.get(feature_name).has_value(),
              std::string(where) + ": unknown feature '" + std::string(feature_name) + "'");
  return frame.features(feature_name);
}

[[nodiscard]] inline bool is_continuous_thread_(const Thread& values) {
  return values.kind() == ThreadKind::Series ||
         values.kind() == ThreadKind::Timeline;
}

[[nodiscard]] inline Thread observed_thread_(const Thread& values) {
  return ::pluss::table::detail::has_validity_mask(values) ? values.dropnull() : values;
}

[[nodiscard]] inline Series continuous_series_from_thread_(
    const Thread& values,
    std::string_view where) {
  PLUSS_CHECK(is_continuous_thread_(values),
              std::string(where) + ": expected a numeric or temporal Thread");
  const auto observed = observed_thread_(values);
  switch (observed.kind()) {
    case ThreadKind::Series:
      return observed.as_series();
    case ThreadKind::Timeline:
      return ::pluss::pl::flash::detail::to_series(observed.as_timeline());
    default:
      PLUSS_FAIL(std::string(where) + ": expected a numeric or temporal Thread");
  }
}

[[nodiscard]] inline Note observed_note_from_thread_(
    const Thread& values,
    std::string_view where) {
  PLUSS_CHECK(values.kind() == ThreadKind::Note,
              std::string(where) + ": expected a text Thread");
  return observed_thread_(values).as_note();
}

[[nodiscard]] inline Series implicit_positions_(std::int64_t n) {
  if (n <= 0) {
    return ::pluss::Series::zeros({0}, ScalarType::Float64);
  }
  return ::pluss::series::linspace(0.0,
                                   static_cast<double>(n - 1),
                                   n,
                                   ScalarType::Float64);
}

[[nodiscard]] inline std::pair<Note, Series> categorical_counts_(
    const Note& categories) {
  std::map<std::string, double> counts;
  for (std::int64_t i = 0; i < categories.numel(); ++i) {
    counts[std::string(categories.at(i))] += 1.0;
  }
  std::vector<std::string> labels;
  std::vector<double> values;
  labels.reserve(counts.size());
  values.reserve(counts.size());
  for (const auto& [label, count] : counts) {
    labels.push_back(label);
    values.push_back(count);
  }
  return {::pluss::make_note(std::move(labels)),
          ::pluss::make_series(values, {static_cast<std::int64_t>(values.size())})};
}

[[nodiscard]] inline std::string resolve_univariate_feature_name_(
    const FlashTableColumns& columns,
    const std::vector<std::string>& available,
    std::string_view where) {
  const auto specified_count =
      static_cast<int>(columns.feature.has_value()) +
      static_cast<int>(columns.x_feature.has_value()) +
      static_cast<int>(columns.y_feature.has_value()) +
      static_cast<int>(columns.z_feature.has_value());

  if (columns.feature.has_value()) {
    PLUSS_CHECK(specified_count == 1,
                std::string(where) +
                    ": univariate selection accepts only one of feature/x_feature/y_feature");
    return *columns.feature;
  }
  if (columns.x_feature.has_value()) {
    PLUSS_CHECK(specified_count == 1,
                std::string(where) +
                    ": univariate selection accepts only one of feature/x_feature/y_feature");
    return *columns.x_feature;
  }
  if (columns.y_feature.has_value()) {
    PLUSS_CHECK(specified_count == 1,
                std::string(where) +
                    ": univariate selection accepts only one of feature/x_feature/y_feature");
    return *columns.y_feature;
  }
  PLUSS_CHECK(!columns.z_feature.has_value(),
              std::string(where) + ": z_feature is only valid for 3D plotting");
  PLUSS_CHECK(
      available.size() == 1,
      std::string(where) +
          ": expected exactly 1 feature or an explicit feature/x_feature/y_feature selection; DataFrame features = " +
          feature_list_(available));
  return available.front();
}

[[nodiscard]] inline std::pair<std::string, Thread> resolve_univariate_feature_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();
  const auto name = resolve_univariate_feature_name_(columns, available, where);
  return {name, require_named_feature_(frame, name, where)};
}

[[nodiscard]] inline std::array<std::pair<std::string, Thread>, 2> resolve_bivariate_features_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();
  if (columns.x_feature.has_value() || columns.y_feature.has_value()) {
    PLUSS_CHECK(columns.x_feature.has_value() && columns.y_feature.has_value(),
                std::string(where) +
                    ": explicit 2D selection requires both x_feature and y_feature");
    PLUSS_CHECK(!columns.feature.has_value() && !columns.z_feature.has_value(),
                std::string(where) +
                    ": 2D selection accepts only x_feature and y_feature");
    return {{
        { *columns.x_feature, require_named_feature_(frame, *columns.x_feature, where) },
        { *columns.y_feature, require_named_feature_(frame, *columns.y_feature, where) },
    }};
  }
  PLUSS_CHECK(!columns.feature.has_value() && !columns.z_feature.has_value(),
              std::string(where) + ": 2D selection accepts only x_feature and y_feature");
  PLUSS_CHECK(
      available.size() == 2,
      std::string(where) +
          ": expected exactly 2 features or explicit x_feature/y_feature; DataFrame features = " +
          feature_list_(available));
  return {{
      { available[0], frame.features(available[0]) },
      { available[1], frame.features(available[1]) },
  }};
}

[[nodiscard]] inline std::array<std::pair<std::string, Thread>, 3> resolve_trivariate_features_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();
  if (columns.x_feature.has_value() || columns.y_feature.has_value() || columns.z_feature.has_value()) {
    PLUSS_CHECK(columns.x_feature.has_value() &&
                    columns.y_feature.has_value() &&
                    columns.z_feature.has_value(),
                std::string(where) +
                    ": explicit 3D selection requires x_feature, y_feature, and z_feature");
    PLUSS_CHECK(!columns.feature.has_value(),
                std::string(where) + ": 3D selection does not accept feature");
    return {{
        { *columns.x_feature, require_named_feature_(frame, *columns.x_feature, where) },
        { *columns.y_feature, require_named_feature_(frame, *columns.y_feature, where) },
        { *columns.z_feature, require_named_feature_(frame, *columns.z_feature, where) },
    }};
  }
  PLUSS_CHECK(!columns.feature.has_value(),
              std::string(where) + ": 3D selection does not accept feature");
  PLUSS_CHECK(
      available.size() == 3,
      std::string(where) +
          ": expected exactly 3 features or explicit x_feature/y_feature/z_feature; DataFrame features = " +
          feature_list_(available));
  return {{
      { available[0], frame.features(available[0]) },
      { available[1], frame.features(available[1]) },
      { available[2], frame.features(available[2]) },
  }};
}

[[nodiscard]] inline std::pair<Series, Series> paired_continuous_series_(
    const Thread& x,
    const Thread& y,
    std::string_view where) {
  PLUSS_CHECK(is_continuous_thread_(x) && is_continuous_thread_(y),
              std::string(where) + ": x and y features must both be numeric or temporal");
  const auto keep = (x.notnull() & y.notnull()).as_series();
  return {
      continuous_series_from_thread_(x.filter(keep), where),
      continuous_series_from_thread_(y.filter(keep), where),
  };
}

[[nodiscard]] inline std::tuple<Series, Series, Series> triplet_continuous_series_(
    const Thread& x,
    const Thread& y,
    const Thread& z,
    std::string_view where) {
  PLUSS_CHECK(is_continuous_thread_(x) && is_continuous_thread_(y) && is_continuous_thread_(z),
              std::string(where) + ": x, y, and z features must all be numeric or temporal");
  const auto keep = (x.notnull() & y.notnull() & z.notnull()).as_series();
  return {
      continuous_series_from_thread_(x.filter(keep), where),
      continuous_series_from_thread_(y.filter(keep), where),
      continuous_series_from_thread_(z.filter(keep), where),
  };
}

[[nodiscard]] inline std::tuple<Series, Series, Series, Series> quadruplet_continuous_series_(
    const Thread& x,
    const Thread& y,
    const Thread& z,
    const Thread& value,
    std::string_view where) {
  PLUSS_CHECK(
      is_continuous_thread_(x) && is_continuous_thread_(y) &&
          is_continuous_thread_(z) && is_continuous_thread_(value),
      std::string(where) + ": x, y, z, and value features must all be numeric or temporal");
  const auto keep = (x.notnull() & y.notnull() & z.notnull() & value.notnull()).as_series();
  return {
      continuous_series_from_thread_(x.filter(keep), where),
      continuous_series_from_thread_(y.filter(keep), where),
      continuous_series_from_thread_(z.filter(keep), where),
      continuous_series_from_thread_(value.filter(keep), where),
  };
}

[[nodiscard]] inline std::pair<Note, Series> grouped_note_series_(
    const Thread& group,
    const Thread& value,
    std::string_view where) {
  PLUSS_CHECK(group.kind() == ThreadKind::Note,
              std::string(where) + ": grouping feature must be text");
  PLUSS_CHECK(is_continuous_thread_(value),
              std::string(where) + ": value feature must be numeric or temporal");
  const auto keep = (group.notnull() & value.notnull()).as_series();
  return {
      observed_note_from_thread_(group.filter(keep), where),
      continuous_series_from_thread_(value.filter(keep), where),
  };
}

[[nodiscard]] inline std::tuple<Note, Series, Note> grouped_note_series_hue_(
    const Thread& group,
    const Thread& value,
    const Thread& hue,
    std::string_view where) {
  PLUSS_CHECK(group.kind() == ThreadKind::Note,
              std::string(where) + ": grouping feature must be text");
  PLUSS_CHECK(is_continuous_thread_(value),
              std::string(where) + ": value feature must be numeric or temporal");
  PLUSS_CHECK(hue.kind() == ThreadKind::Note,
              std::string(where) + ": hue feature must be text");
  const auto keep = (group.notnull() & value.notnull() & hue.notnull()).as_series();
  return {
      observed_note_from_thread_(group.filter(keep), where),
      continuous_series_from_thread_(value.filter(keep), where),
      observed_note_from_thread_(hue.filter(keep), where),
  };
}

inline void set_bivariate_labels_(
    ::pluss::pl::Pane& pane,
    std::string_view x_label,
    std::string_view y_label) {
  pane.x_label(std::string(x_label));
  pane.y_label(std::string(y_label));
}

inline void set_trivariate_labels_(
    ::pluss::pl::Pane& pane,
    std::string_view x_label,
    std::string_view y_label,
    std::string_view z_label) {
  pane.x_label(std::string(x_label));
  pane.y_label(std::string(y_label));
  pane.z_label(std::string(z_label));
}

struct CatFeatureBinding_ {
  std::pair<std::string, Thread> group{};
  std::pair<std::string, Thread> value{};
  std::optional<std::pair<std::string, Thread>> hue{};
};

[[nodiscard]] inline CatFeatureBinding_ resolve_catplot_features_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();
  CatFeatureBinding_ out;

  const bool explicit_group = columns.x_feature.has_value() || columns.label_feature.has_value();
  const bool explicit_value = columns.y_feature.has_value();
  const bool explicit_hue = columns.hue_feature.has_value();

  if (explicit_group || explicit_value || explicit_hue) {
    PLUSS_CHECK(explicit_group && explicit_value,
                std::string(where) +
                    ": categorical plotting requires x_feature (or label_feature) and y_feature");
    const std::string group_name = columns.x_feature.has_value()
        ? *columns.x_feature
        : *columns.label_feature;
    out.group = {group_name, require_named_feature_(frame, group_name, where)};
    out.value = {*columns.y_feature, require_named_feature_(frame, *columns.y_feature, where)};
    if (explicit_hue) {
      out.hue = std::pair<std::string, Thread>{
          *columns.hue_feature,
          require_named_feature_(frame, *columns.hue_feature, where),
      };
    }
    return out;
  }

  PLUSS_CHECK(
      available.size() == 2 || available.size() == 3,
      std::string(where) +
          ": expected exactly 2 features (group, value) or 3 features (group, value, hue), or explicit x_feature/y_feature[/hue_feature]; DataFrame features = " +
          feature_list_(available));
  out.group = {available[0], frame.features(available[0])};
  out.value = {available[1], frame.features(available[1])};
  if (available.size() == 3) {
    out.hue = std::pair<std::string, Thread>{available[2], frame.features(available[2])};
  }
  return out;
}

struct PieBinding_ {
  std::optional<Note> labels{};
  Series values = pluss::series::undefined();
};

[[nodiscard]] inline PieBinding_ resolve_pie_binding_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();

  const auto explicit_single_value = [&]() -> std::optional<std::string> {
    if (columns.feature.has_value()) return columns.feature;
    if (columns.y_feature.has_value() && !columns.x_feature.has_value()) return columns.y_feature;
    return std::nullopt;
  };

  if (columns.label_feature.has_value() ||
      (columns.x_feature.has_value() && columns.y_feature.has_value())) {
    const std::string label_name = columns.label_feature.has_value()
        ? *columns.label_feature
        : *columns.x_feature;
    const std::string value_name = columns.feature.has_value()
        ? *columns.feature
        : *columns.y_feature;
    const auto label_col = require_named_feature_(frame, label_name, where);
    const auto value_col = require_named_feature_(frame, value_name, where);
    const auto [labels, values] = grouped_note_series_(label_col, value_col, where);
    return {labels, values};
  }

  if (const auto value_name = explicit_single_value(); value_name.has_value()) {
    const auto value_col = require_named_feature_(frame, *value_name, where);
    if (value_col.kind() == ThreadKind::Note) {
      const auto categories = observed_note_from_thread_(value_col, where);
      const auto [labels, values] = categorical_counts_(categories);
      return {labels, values};
    }
    return {std::nullopt, continuous_series_from_thread_(value_col, where)};
  }

  if (available.size() == 1) {
    const auto value_col = frame.features(available.front());
    if (value_col.kind() == ThreadKind::Note) {
      const auto categories = observed_note_from_thread_(value_col, where);
      const auto [labels, values] = categorical_counts_(categories);
      return {labels, values};
    }
    return {std::nullopt, continuous_series_from_thread_(value_col, where)};
  }

  PLUSS_CHECK(
      available.size() == 2,
      std::string(where) +
          ": expected exactly 1 feature, an inferable 2-feature {label, value} frame, or explicit label/value selection; DataFrame features = " +
          feature_list_(available));

  const auto left = frame.features(available[0]);
  const auto right = frame.features(available[1]);
  if (left.kind() == ThreadKind::Note && is_continuous_thread_(right)) {
    const auto [labels, values] = grouped_note_series_(left, right, where);
    return {labels, values};
  }
  if (right.kind() == ThreadKind::Note && is_continuous_thread_(left)) {
    const auto [labels, values] = grouped_note_series_(right, left, where);
    return {labels, values};
  }
  PLUSS_FAIL(
      std::string(where) +
      ": 2-feature pie inference requires one text feature and one numeric/temporal feature");
}

struct NumericFrameColumns_ {
  std::vector<Series> columns{};
  std::vector<std::string> labels{};
  Series mask = pluss::series::undefined();
};

[[nodiscard]] inline NumericFrameColumns_ numeric_series_columns_(
    const DataFrame& frame,
    std::string_view where,
    const std::vector<std::string>& excluded_names = {},
    const Thread* extra_required = nullptr) {
  const auto names = frame.features();
  PLUSS_CHECK(!names.empty(), std::string(where) + ": frame has no features");

  std::vector<std::pair<std::string, Thread>> selected;
  selected.reserve(names.size());
  for (const auto& name : names) {
    if (std::find(excluded_names.begin(), excluded_names.end(), name) !=
        excluded_names.end()) {
      continue;
    }
    const auto column = frame.features(name);
    PLUSS_CHECK(
        column.kind() == ThreadKind::Series,
        std::string(where) + ": feature '" + name + "' is not a numeric Series");
    selected.emplace_back(name, column);
  }
  PLUSS_CHECK(!selected.empty(),
              std::string(where) +
                  ": no numeric Series features remain after applying the current selection");

  auto keep = selected.front().second.notnull();
  for (std::size_t i = 1; i < selected.size(); ++i) {
    keep = keep & selected[i].second.notnull();
  }
  if (extra_required != nullptr) {
    keep = keep & extra_required->notnull();
  }
  const auto mask = keep.as_series();

  NumericFrameColumns_ out;
  out.columns.reserve(selected.size());
  out.labels.reserve(selected.size());
  for (const auto& [name, column] : selected) {
    out.labels.push_back(name);
    out.columns.push_back(column.filter(mask).as_series().to(ScalarType::Float64));
  }
  out.mask = mask;
  return out;
}

[[nodiscard]] inline Series pack_numeric_matrix_(
    const std::vector<Series>& columns,
    std::string_view where) {
  PLUSS_CHECK(!columns.empty(), std::string(where) + ": no numeric Series payloads available");
  const auto rows = columns.front().numel();
  auto head = columns.front().reshape({rows, 1});
  if (columns.size() == 1) {
    return head;
  }
  std::vector<Series> tail;
  tail.reserve(columns.size() - 1);
  for (std::size_t i = 1; i < columns.size(); ++i) {
    PLUSS_CHECK(columns[i].numel() == rows,
                std::string(where) + ": numeric Series payload length mismatch");
    tail.push_back(columns[i].reshape({rows, 1}));
  }
  return head.unite(tail, 1);
}

struct MatrixHeatmapPayload_ {
  Series x = pluss::series::undefined();
  Series y = pluss::series::undefined();
  Series z = pluss::series::undefined();
  std::vector<std::string> x_labels{};
};

[[nodiscard]] inline MatrixHeatmapPayload_ matrix_heatmap_payload_(
    const DataFrame& frame,
    std::string_view where) {
  auto numeric = numeric_series_columns_(frame, where);
  PLUSS_CHECK(!numeric.columns.empty(),
              std::string(where) + ": expected at least one numeric Series feature");
  const auto rows = numeric.columns.front().numel();
  const auto cols = static_cast<std::int64_t>(numeric.columns.size());
  return {
      implicit_positions_(cols),
      implicit_positions_(rows),
      pack_numeric_matrix_(numeric.columns, where),
      std::move(numeric.labels),
  };
}

struct HeatmapFeatureBinding_ {
  std::pair<std::string, Thread> x{};
  std::pair<std::string, Thread> y{};
  std::pair<std::string, Thread> z{};
};

[[nodiscard]] inline std::optional<HeatmapFeatureBinding_> resolve_heatmap_features_(
    const DataFrame& frame,
    const FlashTableColumns& columns,
    std::string_view where) {
  const auto available = frame.features();
  if (columns.x_feature.has_value() || columns.y_feature.has_value() ||
      columns.z_feature.has_value() || columns.feature.has_value()) {
    PLUSS_CHECK(columns.x_feature.has_value() && columns.y_feature.has_value(),
                std::string(where) +
                    ": explicit heatmap selection requires x_feature and y_feature");
    const auto z_name = columns.z_feature.has_value() ? columns.z_feature : columns.feature;
    PLUSS_CHECK(z_name.has_value(),
                std::string(where) +
                    ": explicit heatmap selection requires z_feature or feature for levels");
    return HeatmapFeatureBinding_{
        {*columns.x_feature, require_named_feature_(frame, *columns.x_feature, where)},
        {*columns.y_feature, require_named_feature_(frame, *columns.y_feature, where)},
        {*z_name, require_named_feature_(frame, *z_name, where)},
    };
  }
  if (available.size() == 3) {
    return HeatmapFeatureBinding_{
        {available[0], frame.features(available[0])},
        {available[1], frame.features(available[1])},
        {available[2], frame.features(available[2])},
    };
  }
  return std::nullopt;
}

} // namespace detail

[[nodiscard]] inline pluss::pl::flash::Dataset flash_dataset(
    const DataFrame& frame,
    const FlashFromTableOptions& options = {}) {
  PLUSS_CHECK(frame.defined(), "flash::Dataset::from_table: DataFrame is undefined");

  std::vector<std::string> selected = options.features.empty()
      ? frame.features()
      : options.features;

  pluss::pl::flash::Dataset out;
  std::vector<std::string> numeric_names;
  std::vector<std::string> text_names;
  std::vector<std::string> temporal_names;
  std::vector<std::string> feature_order;
  std::vector<std::string> feature_kinds;

  for (const auto& name : selected) {
    PLUSS_CHECK(frame.get(name).has_value(),
                "flash::Dataset::from_table: missing feature '" + name + "'");
    const auto column = frame.features(name);
    const auto mask = detail::compact_validity_mask_(column);

    switch (column.kind()) {
      case ThreadKind::Series:
        PLUSS_CHECK(options.include_series,
                    "flash::Dataset::from_table: series feature '" + name + "' was selected but series export is disabled");
        numeric_names.push_back(name);
        out.numeric_values.push_back(column.as_series());
        out.numeric_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("numeric");
        break;
      case ThreadKind::Note:
        PLUSS_CHECK(options.include_text,
                    "flash::Dataset::from_table: text feature '" + name + "' was selected but text export is disabled");
        text_names.push_back(name);
        out.text_values.push_back(column.as_note());
        out.text_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("text");
        break;
      case ThreadKind::Timeline:
        PLUSS_CHECK(options.include_timeline,
                    "flash::Dataset::from_table: timeline feature '" + name + "' was selected but temporal export is disabled");
        temporal_names.push_back(name);
        out.temporal_values.push_back(column.as_timeline());
        out.temporal_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("temporal");
        break;
      default:
        PLUSS_FAIL("flash::Dataset::from_table: unsupported thread kind for feature '" + name + "'");
    }
  }

  PLUSS_CHECK(!feature_order.empty(), "flash::Dataset::from_table: no exportable features were selected");

  out.feature_order = pluss::make_note(feature_order);
  out.feature_kinds = pluss::make_note(feature_kinds);
  if (!numeric_names.empty()) {
    out.numeric_names = pluss::make_note(numeric_names);
  }
  if (!text_names.empty()) {
    out.text_names = pluss::make_note(text_names);
  }
  if (!temporal_names.empty()) {
    out.temporal_names = pluss::make_note(temporal_names);
  }
  return out;
}

[[nodiscard]] inline DataFrame frame_from_flash_dataset(
    const pluss::pl::flash::Dataset& data,
    const FlashToFrameOptions& options = {}) {
  auto frame = DataFrame(detail::ordered_flash_columns_(data, options));
  if (options.title.has_value()) {
    frame = frame.title(*options.title);
  }
  return frame;
}

} // namespace pluss::table

namespace pluss::pl::flash {
[[nodiscard]] inline Dataset Dataset::from_table(
    const ::pluss::table::DataFrame& frame,
    const FromFrameOptions& options) {
  return ::pluss::table::flash_dataset(frame, options);
}

[[nodiscard]] inline ::pluss::table::DataFrame Dataset::frame(
    const ToFrameOptions& options) const {
  return ::pluss::table::frame_from_flash_dataset(*this, options);
}

inline Trace& lineplot(Pane& pane, const ::pluss::table::Thread& values) {
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::lineplot(Thread)");
  const auto x = ::pluss::table::detail::implicit_positions_(y.numel());
  ::pluss::table::detail::set_bivariate_labels_(pane, "x", "value");
  return lineplot(pane, x, y);
}

inline Trace& lineplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::lineplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::lineplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return lineplot(pane, x, y);
}

inline Trace& lineplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::lineplot(Atlas)");
  return lineplot(pane, frame, columns);
}

inline Trace& scatterplot(Pane& pane, const ::pluss::table::Thread& values) {
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::scatterplot(Thread)");
  const auto x = ::pluss::table::detail::implicit_positions_(y.numel());
  ::pluss::table::detail::set_bivariate_labels_(pane, "x", "value");
  return scatterplot(pane, x, y);
}

inline Trace& scatterplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::scatterplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::scatterplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return scatterplot(pane, x, y);
}

inline Trace& scatterplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::scatterplot(Atlas)");
  return scatterplot(pane, frame, columns);
}

inline Trace& stepplot(Pane& pane, const ::pluss::table::Thread& values) {
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::stepplot(Thread)");
  const auto x = ::pluss::table::detail::implicit_positions_(y.numel());
  ::pluss::table::detail::set_bivariate_labels_(pane, "x", "value");
  return stepplot(pane, x, y);
}

inline Trace& stepplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::stepplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::stepplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return stepplot(pane, x, y);
}

inline Trace& stepplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::stepplot(Atlas)");
  return stepplot(pane, frame, columns);
}

inline Trace& areaplot(
    Pane& pane,
    const ::pluss::table::Thread& values,
    double base) {
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::areaplot(Thread)");
  const auto x = ::pluss::table::detail::implicit_positions_(y.numel());
  ::pluss::table::detail::set_bivariate_labels_(pane, "x", "value");
  return areaplot(pane, x, y, base);
}

inline Trace& areaplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    double base) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::areaplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::areaplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return areaplot(pane, x, y, base);
}

inline Trace& areaplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    double base) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::areaplot(Atlas)");
  return areaplot(pane, frame, columns, base);
}

inline Trace& histplot(Pane& pane, const ::pluss::table::Thread& values, int bins) {
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::histplot(Thread)");
  pane.x_label("x").y_label("count");
  return histplot(pane, x, bins);
}

inline Trace& histplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    int bins) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::histplot(DataFrame)");
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::histplot(DataFrame)");
  pane.x_label(name).y_label("count");
  return histplot(pane, x, bins);
}

inline Trace& histplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    int bins) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::histplot(Atlas)");
  return histplot(pane, frame, columns, bins);
}

inline Trace& displot(Pane& pane, const ::pluss::table::Thread& values, int bins) {
  return histplot(pane, values, bins);
}

inline Trace& displot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    int bins) {
  return histplot(pane, frame, columns, bins);
}

inline Trace& displot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    int bins) {
  return histplot(pane, atlas, columns, bins);
}

inline Trace& kdeplot(
    Pane& pane,
    const ::pluss::table::Thread& values,
    std::int64_t points) {
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::kdeplot(Thread)");
  pane.x_label("x").y_label("density");
  return kdeplot(pane, x, points);
}

inline Trace& kdeplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    std::int64_t points) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::kdeplot(DataFrame)");
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::kdeplot(DataFrame)");
  pane.x_label(name).y_label("density");
  return kdeplot(pane, x, points);
}

inline Trace& kdeplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    std::int64_t points) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::kdeplot(Atlas)");
  return kdeplot(pane, frame, columns, points);
}

inline Trace& ecdfplot(Pane& pane, const ::pluss::table::Thread& values) {
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::ecdfplot(Thread)");
  pane.x_label("x").y_label("cdf");
  return ecdfplot(pane, x);
}

inline Trace& ecdfplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::ecdfplot(DataFrame)");
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::ecdfplot(DataFrame)");
  pane.x_label(name).y_label("cdf");
  return ecdfplot(pane, x);
}

inline Trace& ecdfplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::ecdfplot(Atlas)");
  return ecdfplot(pane, frame, columns);
}

inline Trace& rugplot(
    Pane& pane,
    const ::pluss::table::Thread& values,
    double height) {
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::rugplot(Thread)");
  pane.x_label("x");
  return rugplot(pane, x, height);
}

inline Trace& rugplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    double height) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::rugplot(DataFrame)");
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::rugplot(DataFrame)");
  pane.x_label(name);
  return rugplot(pane, x, height);
}

inline Trace& rugplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    double height) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::rugplot(Atlas)");
  return rugplot(pane, frame, columns, height);
}

inline Trace& countplot(Pane& pane, const ::pluss::table::Thread& values, int bins) {
  if (values.kind() == ::pluss::table::ThreadKind::Note) {
    const auto categories = ::pluss::table::detail::observed_note_from_thread_(
        values, "flash::countplot(Thread)");
    pane.x_label("x").y_label("count");
    return countplot(pane, categories);
  }
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::countplot(Thread)");
  pane.x_label("x").y_label("count");
  return countplot(pane, x, bins);
}

inline Trace& countplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    int bins) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::countplot(DataFrame)");
  if (column.kind() == ::pluss::table::ThreadKind::Note) {
    const auto categories = ::pluss::table::detail::observed_note_from_thread_(
        column, "flash::countplot(DataFrame)");
    pane.x_label(name).y_label("count");
    return countplot(pane, categories);
  }
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::countplot(DataFrame)");
  pane.x_label(name).y_label("count");
  return countplot(pane, x, bins);
}

inline Trace& countplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    int bins) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::countplot(Atlas)");
  return countplot(pane, frame, columns, bins);
}

inline Trace& barplot(
    Pane& pane,
    const ::pluss::table::Thread& values,
    const BarPlotOptions& options) {
  if (values.kind() == ::pluss::table::ThreadKind::Note) {
    const auto categories = ::pluss::table::detail::observed_note_from_thread_(
        values, "flash::barplot(Thread)");
    const auto [labels, counts] = ::pluss::table::detail::categorical_counts_(categories);
    if (options.horizontal) {
      pane.x_label("count").y_label("y");
    } else {
      pane.x_label("x").y_label("count");
    }
    return barplot(pane, labels, counts, options);
  }
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      values, "flash::barplot(Thread)");
  const auto x = ::pluss::table::detail::implicit_positions_(y.numel());
  if (options.horizontal) {
    pane.x_label("value").y_label("y");
  } else {
    pane.x_label("x").y_label("value");
  }
  return barplot(pane, x, y, options);
}

inline Trace& barplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    const BarPlotOptions& options) {
  const auto available = frame.features();
  const bool use_bivariate =
      (columns.x_feature.has_value() || columns.y_feature.has_value()) ||
      (available.size() == 2 && !columns.feature.has_value() && !columns.z_feature.has_value());
  if (use_bivariate) {
    const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
        frame, columns, "flash::barplot(DataFrame)");
    if (::pluss::table::detail::is_continuous_thread_(x_col.second) &&
        ::pluss::table::detail::is_continuous_thread_(y_col.second)) {
      const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
          x_col.second, y_col.second, "flash::barplot(DataFrame)");
      if (options.horizontal) {
        ::pluss::table::detail::set_bivariate_labels_(pane, y_col.first, x_col.first);
      } else {
        ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
      }
      return barplot(pane, x, y, options);
    }

    if (x_col.second.kind() == ::pluss::table::ThreadKind::Note &&
        ::pluss::table::detail::is_continuous_thread_(y_col.second)) {
      const auto [labels, values] = ::pluss::table::detail::grouped_note_series_(
          x_col.second, y_col.second, "flash::barplot(DataFrame)");
      if (options.horizontal) {
        ::pluss::table::detail::set_bivariate_labels_(pane, y_col.first, x_col.first);
      } else {
        ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
      }
      return barplot(pane, labels, values, options);
    }

    if (y_col.second.kind() == ::pluss::table::ThreadKind::Note &&
        ::pluss::table::detail::is_continuous_thread_(x_col.second)) {
      const auto [labels, values] = ::pluss::table::detail::grouped_note_series_(
          y_col.second, x_col.second, "flash::barplot(DataFrame)");
      if (options.horizontal) {
        ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
      } else {
        ::pluss::table::detail::set_bivariate_labels_(pane, y_col.first, x_col.first);
      }
      return barplot(pane, labels, values, options);
    }

    PLUSS_FAIL(
        "flash::barplot(DataFrame): 2D bar plots require numeric/temporal pairs or one text axis with one numeric/temporal value axis");
  }

  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::barplot(DataFrame)");
  if (column.kind() == ::pluss::table::ThreadKind::Note) {
    const auto categories = ::pluss::table::detail::observed_note_from_thread_(
        column, "flash::barplot(DataFrame)");
    const auto [labels, counts] = ::pluss::table::detail::categorical_counts_(categories);
    if (options.horizontal) {
      pane.x_label("count").y_label(name);
    } else {
      pane.x_label(name).y_label("count");
    }
    return barplot(pane, labels, counts, options);
  }

  const auto values = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::barplot(DataFrame)");
  const auto positions = ::pluss::table::detail::implicit_positions_(values.numel());
  if (options.horizontal) {
    pane.x_label(name).y_label("y");
  } else {
    pane.x_label("x").y_label(name);
  }
  return barplot(pane, positions, values, options);
}

inline Trace& barplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    const BarPlotOptions& options) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::barplot(Atlas)");
  return barplot(pane, frame, columns, options);
}

inline Trace& path3(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::path3(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::path3(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(
      pane, x_col.first, y_col.first, z_col.first);
  return path3(pane, x, y, z);
}

inline Trace& path3(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::path3(Atlas)");
  return path3(pane, frame, columns);
}

inline Trace& cloud3(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::cloud3(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::cloud3(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(
      pane, x_col.first, y_col.first, z_col.first);
  return cloud3(pane, x, y, z);
}

inline Trace& cloud3(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::cloud3(Atlas)");
  return cloud3(pane, frame, columns);
}

inline Trace& plot3D(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  return path3(pane, frame, columns);
}

inline Trace& plot3D(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  return path3(pane, atlas, columns);
}

inline Trace& scatter3D(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns) {
  return cloud3(pane, frame, columns);
}

inline Trace& scatter3D(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns) {
  return cloud3(pane, atlas, columns);
}

inline Trace& relplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    std::string_view kind = "scatter") {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::relplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  if (!columns.hue_feature.has_value()) {
    const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
        x_col.second, y_col.second, "flash::relplot(DataFrame)");
    return relplot(pane, x, y, kind);
  }

  const auto hue_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.hue_feature, "flash::relplot(DataFrame)");
  const auto keep = (x_col.second.notnull() & y_col.second.notnull() & hue_col.notnull()).as_series();
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      x_col.second.filter(keep), "flash::relplot(DataFrame)");
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      y_col.second.filter(keep), "flash::relplot(DataFrame)");
  if (hue_col.kind() == ::pluss::table::ThreadKind::Note) {
    const auto hue = ::pluss::table::detail::observed_note_from_thread_(
        hue_col.filter(keep), "flash::relplot(DataFrame)");
    return relplot(pane, x, y, hue, kind);
  }
  PLUSS_CHECK(
      ::pluss::table::detail::is_continuous_thread_(hue_col),
      "flash::relplot(DataFrame): hue_feature must be text, numeric, or temporal");
  const auto hue = ::pluss::table::detail::continuous_series_from_thread_(
      hue_col.filter(keep), "flash::relplot(DataFrame)");
  return relplot(pane, x, y, hue, kind);
}

inline Trace& relplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    std::string_view kind = "scatter") {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::relplot(Atlas)");
  return relplot(pane, frame, columns, kind);
}

inline Trace& relplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    std::string_view kind = "scatter",
    std::string title = {}) {
  auto& pane = studio.pane2(std::move(title));
  return relplot(pane, frame, columns, kind);
}

inline Trace& relplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    std::string_view kind = "scatter",
    std::string title = {}) {
  auto& pane = studio.pane2(std::move(title));
  return relplot(pane, atlas, columns, kind);
}

inline Studio& jointplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    std::string_view kind) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::jointplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::jointplot(DataFrame)");
  return jointplot(studio, x, y, kind);
}

inline Studio& jointplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    const JointPlotOptions& options) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::jointplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::jointplot(DataFrame)");
  return jointplot(studio, x, y, options);
}

inline Studio& jointplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    std::string_view kind) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::jointplot(Atlas)");
  return jointplot(studio, frame, columns, kind);
}

inline Studio& jointplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    const JointPlotOptions& options) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::jointplot(Atlas)");
  return jointplot(studio, frame, columns, options);
}

inline Trace& stripplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::stripplot(DataFrame)");
  PLUSS_CHECK(!binding.hue.has_value(),
              "flash::stripplot(DataFrame): hue_feature is not supported; use flash::catplot(..., options)");
  const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
      binding.group.second, binding.value.second, "flash::stripplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  return stripplot(pane, group, values);
}

inline Trace& stripplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::stripplot(Atlas)");
  return stripplot(pane, frame, columns);
}

inline Trace& swarmplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::swarmplot(DataFrame)");
  PLUSS_CHECK(!binding.hue.has_value(),
              "flash::swarmplot(DataFrame): hue_feature is not supported; use flash::catplot(..., options)");
  const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
      binding.group.second, binding.value.second, "flash::swarmplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  return swarmplot(pane, group, values);
}

inline Trace& swarmplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::swarmplot(Atlas)");
  return swarmplot(pane, frame, columns);
}

inline Trace& boxplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::boxplot(DataFrame)");
  if (!binding.hue.has_value()) {
    const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
        binding.group.second, binding.value.second, "flash::boxplot(DataFrame)");
    ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
    return boxplot(pane, group, values);
  }
  const auto [group, values, hue] = ::pluss::table::detail::grouped_note_series_hue_(
      binding.group.second, binding.value.second, binding.hue->second, "flash::boxplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  CatPlotOptions options;
  options.kind = CatPlotKind::Box;
  options.dodge = true;
  return catplot(pane, group, values, hue, options);
}

inline Trace& boxplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::boxplot(Atlas)");
  return boxplot(pane, frame, columns);
}

inline Trace& violinplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::violinplot(DataFrame)");
  if (!binding.hue.has_value()) {
    const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
        binding.group.second, binding.value.second, "flash::violinplot(DataFrame)");
    ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
    return violinplot(pane, group, values);
  }
  const auto [group, values, hue] = ::pluss::table::detail::grouped_note_series_hue_(
      binding.group.second, binding.value.second, binding.hue->second, "flash::violinplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  CatPlotOptions options;
  options.kind = CatPlotKind::Violin;
  options.dodge = true;
  return catplot(pane, group, values, hue, options);
}

inline Trace& violinplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    const ViolinOptions& options) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::violinplot(DataFrame, options)");
  PLUSS_CHECK(!binding.hue.has_value(),
              "flash::violinplot(DataFrame, options): hue_feature is not supported with direct violin options; use flash::catplot");
  const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
      binding.group.second, binding.value.second, "flash::violinplot(DataFrame, options)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  return violinplot(pane, group, values, options);
}

inline Trace& violinplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::violinplot(Atlas)");
  return violinplot(pane, frame, columns);
}

inline Trace& violinplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    const ViolinOptions& options) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::violinplot(Atlas, options)");
  return violinplot(pane, frame, columns, options);
}

inline Trace& boxenplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::boxenplot(DataFrame)");
  const auto values = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::boxenplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, name, "value");
  return boxenplot(pane, values);
}

inline Trace& boxenplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::boxenplot(Atlas)");
  return boxenplot(pane, frame, columns);
}

inline Trace& catplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns,
    const CatPlotOptions& options) {
  const auto binding = ::pluss::table::detail::resolve_catplot_features_(
      frame, columns, "flash::catplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, binding.group.first, binding.value.first);
  if (!binding.hue.has_value()) {
    const auto [group, values] = ::pluss::table::detail::grouped_note_series_(
        binding.group.second, binding.value.second, "flash::catplot(DataFrame)");
    return catplot(pane, group, values, options);
  }
  const auto [group, values, hue] = ::pluss::table::detail::grouped_note_series_hue_(
      binding.group.second, binding.value.second, binding.hue->second, "flash::catplot(DataFrame)");
  return catplot(pane, group, values, hue, options);
}

inline Trace& catplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    std::string_view kind = "strip") {
  CatPlotOptions options;
  options.kind = ::pluss::pl::flash::detail::catplot_kind(kind);
  return catplot(pane, frame, columns, options);
}

inline Trace& catplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns,
    const CatPlotOptions& options) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::catplot(Atlas)");
  return catplot(pane, frame, columns, options);
}

inline Trace& catplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    std::string_view kind = "strip") {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::catplot(Atlas)");
  return catplot(pane, frame, columns, kind);
}

inline Trace& pieplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto binding = ::pluss::table::detail::resolve_pie_binding_(
      frame, columns, "flash::pieplot(DataFrame)");
  if (!binding.labels.has_value()) {
    return pieplot(pane, binding.values);
  }
  const auto labels = ::pluss::pl::flash::detail::note_values(*binding.labels);
  const auto label_views = ::pluss::pl::flash::detail::label_views(labels);
  return pieplot(pane, binding.values, pluss::msharp::RangeView::Of<std::string_view>(label_views));
}

inline Trace& pieplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::pieplot(Atlas)");
  return pieplot(pane, frame, columns);
}

inline Trace& regplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::regplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::regplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return regplot(pane, x, y, options);
}

inline Trace& regplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::regplot(Atlas)");
  return regplot(pane, frame, columns, options);
}

inline Trace& residplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::residplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::residplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return residplot(pane, x, y, options);
}

inline Trace& residplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::residplot(Atlas)");
  return residplot(pane, frame, columns, options);
}

inline Trace& qqplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const QqPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::qqplot(DataFrame)");
  const auto values = ::pluss::table::detail::continuous_series_from_thread_(
      column, "flash::qqplot(DataFrame)");
  pane.x_label("theoretical quantiles").y_label(name);
  return qqplot(pane, values, options);
}

inline Trace& qqplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const QqPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::qqplot(Atlas)");
  return qqplot(pane, frame, columns, options);
}

inline Trace& heatmap(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const HeatmapOptions& options = {}) {
  if (const auto tall = ::pluss::table::detail::resolve_heatmap_features_(
          frame, columns, "flash::heatmap(DataFrame)");
      tall.has_value()) {
    const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
        tall->x.second, tall->y.second, tall->z.second, "flash::heatmap(DataFrame)");
    const auto [grid_x, grid_y, grid_z] = ::pluss::pl::flash::detail::triplot_bin_surface(
        x, y, z);
    ::pluss::table::detail::set_trivariate_labels_(pane, tall->x.first, tall->y.first, tall->z.first);
    return heatmap(pane, grid_x, grid_y, grid_z, options);
  }
  auto matrix = ::pluss::table::detail::matrix_heatmap_payload_(
      frame, "flash::heatmap(DataFrame)");
  const auto label_views = ::pluss::pl::flash::detail::label_views(matrix.x_labels);
  pane.x_ticks(matrix.x.to_vec<double>(), pluss::msharp::RangeView::Of<std::string_view>(label_views))
      .x_label("feature")
      .y_label("case");
  return heatmap(pane, matrix.x, matrix.y, matrix.z, options);
}

inline Trace& heatmap(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const HeatmapOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::heatmap(Atlas)");
  return heatmap(pane, frame, columns, options);
}

inline Trace& clustermap(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const ClusterMapOptions& options = {}) {
  if (const auto tall = ::pluss::table::detail::resolve_heatmap_features_(
          frame, columns, "flash::clustermap(DataFrame)");
      tall.has_value()) {
    const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
        tall->x.second, tall->y.second, tall->z.second, "flash::clustermap(DataFrame)");
    ::pluss::table::detail::set_trivariate_labels_(pane, tall->x.first, tall->y.first, tall->z.first);
    return clustermap(pane, x, y, z, options);
  }
  auto matrix = ::pluss::table::detail::matrix_heatmap_payload_(
      frame, "flash::clustermap(DataFrame)");
  const auto label_views = ::pluss::pl::flash::detail::label_views(matrix.x_labels);
  pane.x_ticks(matrix.x.to_vec<double>(), pluss::msharp::RangeView::Of<std::string_view>(label_views))
      .x_label("feature")
      .y_label("case");
  return clustermap(pane, matrix.x, matrix.y, matrix.z, options);
}

inline Trace& clustermap(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const ClusterMapOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::clustermap(Atlas)");
  return clustermap(pane, frame, columns, options);
}

inline Trace& sheet(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::sheet(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::sheet(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(pane, x_col.first, y_col.first, z_col.first);
  return sheet(pane, x, y, z);
}

inline Trace& sheet(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::sheet(Atlas)");
  return sheet(pane, frame, columns);
}

inline Trace& lattice(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::lattice(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::lattice(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(pane, x_col.first, y_col.first, z_col.first);
  return lattice(pane, x, y, z);
}

inline Trace& lattice(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::lattice(Atlas)");
  return lattice(pane, frame, columns);
}

inline Trace& spire3(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::spire3(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::spire3(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(pane, x_col.first, y_col.first, z_col.first);
  return spire3(pane, x, y, z);
}

inline Trace& spire3(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::spire3(Atlas)");
  return spire3(pane, frame, columns);
}

inline Trace& ridge3(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    int level_count = 8) {
  const auto [x_col, y_col, z_col] = ::pluss::table::detail::resolve_trivariate_features_(
      frame, columns, "flash::ridge3(DataFrame)");
  const auto [x, y, z] = ::pluss::table::detail::triplet_continuous_series_(
      x_col.second, y_col.second, z_col.second, "flash::ridge3(DataFrame)");
  ::pluss::table::detail::set_trivariate_labels_(pane, x_col.first, y_col.first, z_col.first);
  return ridge3(pane, x, y, z, level_count);
}

inline Trace& ridge3(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    int level_count = 8) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::ridge3(Atlas)");
  return ridge3(pane, frame, columns, level_count);
}

inline Trace& plot_surface(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  return sheet(pane, frame, columns);
}

inline Trace& plot_surface(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  return sheet(pane, atlas, columns);
}

inline Trace& plot_wireframe(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  return lattice(pane, frame, columns);
}

inline Trace& plot_wireframe(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  return lattice(pane, atlas, columns);
}

inline Trace& contour3D(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    int level_count = 8) {
  return ridge3(pane, frame, columns, level_count);
}

inline Trace& contour3D(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    int level_count = 8) {
  return ridge3(pane, atlas, columns, level_count);
}

inline Trace& bar3d(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  return spire3(pane, frame, columns);
}

inline Trace& bar3d(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  return spire3(pane, atlas, columns);
}

inline Trace& isoshell(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    int level_count = 4) {
  PLUSS_CHECK(
      columns.x_feature.has_value() && columns.y_feature.has_value() &&
          columns.z_feature.has_value() && columns.feature.has_value(),
      "flash::isoshell(DataFrame): explicit x_feature, y_feature, z_feature, and feature are required");
  const auto x_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.x_feature, "flash::isoshell(DataFrame)");
  const auto y_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.y_feature, "flash::isoshell(DataFrame)");
  const auto z_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.z_feature, "flash::isoshell(DataFrame)");
  const auto value_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.feature, "flash::isoshell(DataFrame)");
  const auto [x, y, z, values] = ::pluss::table::detail::quadruplet_continuous_series_(
      x_col, y_col, z_col, value_col, "flash::isoshell(DataFrame)");
  pane.x_label(*columns.x_feature).y_label(*columns.y_feature).z_label(*columns.z_feature);
  return isoshell(pane, x, y, z, values, level_count);
}

inline Trace& isoshell(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    int level_count = 4) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::isoshell(Atlas)");
  return isoshell(pane, frame, columns, level_count);
}

inline Trace& hist3d(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const Hist3DOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::hist3d(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::hist3d(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, x_col.first, y_col.first);
  return hist3d(pane, x, y, options);
}

inline Trace& hist3d(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const Hist3DOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::hist3d(Atlas)");
  return hist3d(pane, frame, columns, options);
}

inline Trace& pointplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  PLUSS_CHECK(columns.x_feature.has_value() && columns.y_feature.has_value(),
              "flash::pointplot(DataFrame): x_feature and y_feature are required");
  const auto x_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.x_feature, "flash::pointplot(DataFrame)");
  const auto y_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.y_feature, "flash::pointplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col, y_col, "flash::pointplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, *columns.x_feature, *columns.y_feature);
  if (columns.err_feature.has_value()) {
    const auto err_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.err_feature, "flash::pointplot(DataFrame)");
    const auto keep = (x_col.notnull() & y_col.notnull() & err_col.notnull()).as_series();
    const auto err = ::pluss::table::detail::continuous_series_from_thread_(
        err_col.filter(keep), "flash::pointplot(DataFrame)");
    return pointplot(pane, x, y, err);
  }
  return lineplot(pane, x, y);
}

inline Trace& whiskplot(
    Pane& pane,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {}) {
  PLUSS_CHECK(columns.x_feature.has_value() && columns.y_feature.has_value(),
              "flash::whiskplot(DataFrame): x_feature and y_feature are required");
  const auto x_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.x_feature, "flash::whiskplot(DataFrame)");
  const auto y_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.y_feature, "flash::whiskplot(DataFrame)");
  ::pluss::table::detail::set_bivariate_labels_(pane, *columns.x_feature, *columns.y_feature);

  if (columns.err_feature.has_value()) {
    const auto err_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.err_feature, "flash::whiskplot(DataFrame)");
    const auto keep = (x_col.notnull() & y_col.notnull() & err_col.notnull()).as_series();
    const auto x = ::pluss::table::detail::continuous_series_from_thread_(
        x_col.filter(keep), "flash::whiskplot(DataFrame)");
    const auto y = ::pluss::table::detail::continuous_series_from_thread_(
        y_col.filter(keep), "flash::whiskplot(DataFrame)");
    const auto err = ::pluss::table::detail::continuous_series_from_thread_(
        err_col.filter(keep), "flash::whiskplot(DataFrame)");
    return whiskplot(pane, x, y, err);
  }

  PLUSS_CHECK(columns.yneg_feature.has_value() && columns.ypos_feature.has_value(),
              "flash::whiskplot(DataFrame): either err_feature or both yneg_feature/ypos_feature are required");
  const auto yneg_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.yneg_feature, "flash::whiskplot(DataFrame)");
  const auto ypos_col = ::pluss::table::detail::require_named_feature_(
      frame, *columns.ypos_feature, "flash::whiskplot(DataFrame)");
  auto keep = (x_col.notnull() & y_col.notnull() & yneg_col.notnull() & ypos_col.notnull()).as_series();
  std::optional<::pluss::table::Thread> xneg_col;
  std::optional<::pluss::table::Thread> xpos_col;
  if (columns.xneg_feature.has_value()) {
    xneg_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.xneg_feature, "flash::whiskplot(DataFrame)");
    keep = keep.logical_and(xneg_col->notnull().as_series());
  }
  if (columns.xpos_feature.has_value()) {
    xpos_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.xpos_feature, "flash::whiskplot(DataFrame)");
    keep = keep.logical_and(xpos_col->notnull().as_series());
  }
  const auto x = ::pluss::table::detail::continuous_series_from_thread_(
      x_col.filter(keep), "flash::whiskplot(DataFrame)");
  const auto y = ::pluss::table::detail::continuous_series_from_thread_(
      y_col.filter(keep), "flash::whiskplot(DataFrame)");
  const auto yneg = ::pluss::table::detail::continuous_series_from_thread_(
      yneg_col.filter(keep), "flash::whiskplot(DataFrame)");
  const auto ypos = ::pluss::table::detail::continuous_series_from_thread_(
      ypos_col.filter(keep), "flash::whiskplot(DataFrame)");
  const auto xneg = columns.xneg_feature.has_value()
      ? ::pluss::table::detail::continuous_series_from_thread_(
            xneg_col->filter(keep), "flash::whiskplot(DataFrame)")
      : ::pluss::Series::zeros({x.numel()}, ScalarType::Float64);
  const auto xpos = columns.xpos_feature.has_value()
      ? ::pluss::table::detail::continuous_series_from_thread_(
            xpos_col->filter(keep), "flash::whiskplot(DataFrame)")
      : ::pluss::Series::zeros({x.numel()}, ScalarType::Float64);
  return whiskplot(pane, x, y, yneg, ypos, xneg, xpos);
}

inline Trace& pointplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::pointplot(Atlas)");
  return pointplot(pane, frame, columns);
}

inline Trace& whiskplot(
    Pane& pane,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::whiskplot(Atlas)");
  return whiskplot(pane, frame, columns);
}

inline Trace& lmplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {},
    std::string title = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::lmplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::lmplot(DataFrame)");
  return lmplot(
      studio,
      x,
      y,
      options,
      std::move(title));
}

inline Trace& lmplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const RegPlotOptions& options = {},
    std::string title = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::lmplot(Atlas)");
  return lmplot(studio, frame, columns, options, std::move(title));
}

inline Studio& lagplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const LagPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::lagplot(DataFrame)");
  (void)name;
  return lagplot(studio,
                 ::pluss::table::detail::continuous_series_from_thread_(column, "flash::lagplot(DataFrame)"),
                 options);
}

inline Studio& lagplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const LagPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::lagplot(Atlas)");
  return lagplot(studio, frame, columns, options);
}

inline Studio& acfplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const AcfPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::acfplot(DataFrame)");
  (void)name;
  return acfplot(studio,
                 ::pluss::table::detail::continuous_series_from_thread_(column, "flash::acfplot(DataFrame)"),
                 options);
}

inline Studio& acfplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const AcfPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::acfplot(Atlas)");
  return acfplot(studio, frame, columns, options);
}

inline Studio& pacfplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const PacfPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::pacfplot(DataFrame)");
  (void)name;
  return pacfplot(studio,
                  ::pluss::table::detail::continuous_series_from_thread_(column, "flash::pacfplot(DataFrame)"),
                  options);
}

inline Studio& pacfplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const PacfPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::pacfplot(Atlas)");
  return pacfplot(studio, frame, columns, options);
}

inline Studio& actplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const AutocorrelationPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::actplot(DataFrame)");
  (void)name;
  return actplot(studio,
                 ::pluss::table::detail::continuous_series_from_thread_(column, "flash::actplot(DataFrame)"),
                 options);
}

inline Studio& actplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const AutocorrelationPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::actplot(Atlas)");
  return actplot(studio, frame, columns, options);
}

inline Studio& bootstrapplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const BootstrapPlotOptions& options = {}) {
  const auto [name, column] = ::pluss::table::detail::resolve_univariate_feature_(
      frame, columns, "flash::bootstrapplot(DataFrame)");
  (void)name;
  return bootstrapplot(
      studio,
      ::pluss::table::detail::continuous_series_from_thread_(column, "flash::bootstrapplot(DataFrame)"),
      options);
}

inline Studio& bootstrapplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const BootstrapPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::bootstrapplot(Atlas)");
  return bootstrapplot(studio, frame, columns, options);
}

inline Studio& ccfplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const CcfPlotOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::ccfplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::ccfplot(DataFrame)");
  return ccfplot(studio, x, y, options);
}

inline Studio& ccfplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const CcfPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::ccfplot(Atlas)");
  return ccfplot(studio, frame, columns, options);
}

inline Trace& resid_histplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const ResidHistPlotOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::resid_histplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::resid_histplot(DataFrame)");
  return resid_histplot(studio, x, y, options);
}

inline Trace& resid_histplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const ResidHistPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::resid_histplot(Atlas)");
  return resid_histplot(studio, frame, columns, options);
}

inline Studio& resid_acfplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const ResidAcfPlotOptions& options = {}) {
  const auto [x_col, y_col] = ::pluss::table::detail::resolve_bivariate_features_(
      frame, columns, "flash::resid_acfplot(DataFrame)");
  const auto [x, y] = ::pluss::table::detail::paired_continuous_series_(
      x_col.second, y_col.second, "flash::resid_acfplot(DataFrame)");
  return resid_acfplot(studio, x, y, options);
}

inline Studio& resid_acfplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const ResidAcfPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::resid_acfplot(Atlas)");
  return resid_acfplot(studio, frame, columns, options);
}

inline Studio& pairplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const PairPlotOptions& options = {}) {
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::pairplot(DataFrame)");
  auto local_options = options;
  if (local_options.labels.empty()) {
    local_options.labels = numeric.labels;
  }
  return pairplot(studio, numeric.columns, local_options);
}

inline Studio& pairplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const PairPlotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::pairplot(Atlas)");
  return pairplot(studio, frame, options);
}

inline Studio& triplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TriplotOptions& options = {}) {
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::triplot(DataFrame)");
  auto local_options = options;
  if (local_options.labels.empty()) {
    local_options.labels = numeric.labels;
  }
  return triplot(studio, numeric.columns, local_options);
}

inline Studio& triplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TriplotOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::triplot(Atlas)");
  return triplot(studio, frame, options);
}

inline Studio& parallelplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const ParallelCoordinatesOptions& options = {}) {
  std::vector<std::string> excluded;
  std::optional<::pluss::table::Thread> class_col;
  const ::pluss::table::Thread* class_ptr = nullptr;
  if (columns.class_feature.has_value()) {
    class_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.class_feature, "flash::parallelplot(DataFrame)");
    PLUSS_CHECK(class_col->kind() == ::pluss::table::ThreadKind::Note,
                "flash::parallelplot(DataFrame): class_feature must be a text feature");
    excluded.push_back(*columns.class_feature);
    class_ptr = &*class_col;
  }
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::parallelplot(DataFrame)", excluded, class_ptr);
  auto local_options = options;
  if (local_options.labels.empty()) {
    local_options.labels = numeric.labels;
  }
  if (!class_col.has_value()) {
    return parallelplot(studio, numeric.columns, local_options);
  }
  const auto labels = ::pluss::table::detail::observed_note_from_thread_(
      class_col->filter(numeric.mask), "flash::parallelplot(DataFrame)");
  return parallelplot(studio, numeric.columns, labels, local_options);
}

inline Studio& parallelplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const ParallelCoordinatesOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::parallelplot(Atlas)");
  return parallelplot(studio, frame, columns, options);
}

inline Studio& andrews_curves(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const AndrewsCurvesOptions& options = {}) {
  std::vector<std::string> excluded;
  std::optional<::pluss::table::Thread> class_col;
  const ::pluss::table::Thread* class_ptr = nullptr;
  if (columns.class_feature.has_value()) {
    class_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.class_feature, "flash::andrews_curves(DataFrame)");
    PLUSS_CHECK(class_col->kind() == ::pluss::table::ThreadKind::Note,
                "flash::andrews_curves(DataFrame): class_feature must be a text feature");
    excluded.push_back(*columns.class_feature);
    class_ptr = &*class_col;
  }
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::andrews_curves(DataFrame)", excluded, class_ptr);
  auto local_options = options;
  if (local_options.labels.empty()) {
    local_options.labels = numeric.labels;
  }
  if (!class_col.has_value()) {
    return andrews_curves(studio, numeric.columns, local_options);
  }
  const auto labels = ::pluss::table::detail::observed_note_from_thread_(
      class_col->filter(numeric.mask), "flash::andrews_curves(DataFrame)");
  return andrews_curves(studio, numeric.columns, labels, local_options);
}

inline Studio& andrews_curves(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const AndrewsCurvesOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::andrews_curves(Atlas)");
  return andrews_curves(studio, frame, columns, options);
}

inline Studio& radviz(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const TableColumns& columns = {},
    const RadvizOptions& options = {}) {
  std::vector<std::string> excluded;
  std::optional<::pluss::table::Thread> class_col;
  const ::pluss::table::Thread* class_ptr = nullptr;
  if (columns.class_feature.has_value()) {
    class_col = ::pluss::table::detail::require_named_feature_(
        frame, *columns.class_feature, "flash::radviz(DataFrame)");
    PLUSS_CHECK(class_col->kind() == ::pluss::table::ThreadKind::Note,
                "flash::radviz(DataFrame): class_feature must be a text feature");
    excluded.push_back(*columns.class_feature);
    class_ptr = &*class_col;
  }
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::radviz(DataFrame)", excluded, class_ptr);
  auto local_options = options;
  if (local_options.labels.empty()) {
    local_options.labels = numeric.labels;
  }
  if (!class_col.has_value()) {
    return radviz(studio, numeric.columns, local_options);
  }
  const auto labels = ::pluss::table::detail::observed_note_from_thread_(
      class_col->filter(numeric.mask), "flash::radviz(DataFrame)");
  return radviz(studio, numeric.columns, labels, local_options);
}

inline Studio& radviz(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const TableColumns& columns = {},
    const RadvizOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::radviz(Atlas)");
  return radviz(studio, frame, columns, options);
}

inline Studio& summaryplot(
    Studio& studio,
    const ::pluss::table::DataFrame& frame,
    const SummaryOptions& options = {}) {
  auto numeric = ::pluss::table::detail::numeric_series_columns_(
      frame, "flash::summaryplot(DataFrame)");
  const auto label_views = ::pluss::pl::flash::detail::label_views(numeric.labels);
  return summaryplot(
      studio,
      pluss::msharp::RangeView::Series(numeric.columns),
      pluss::msharp::RangeView::Of<std::string_view>(label_views),
      options);
}

inline Studio& summaryplot(
    Studio& studio,
    const ::pluss::table::Atlas& atlas,
    const SummaryOptions& options = {}) {
  const auto frame = ::pluss::table::detail::single_frame_from_atlas_(
      atlas, "flash::summaryplot(Atlas)");
  return summaryplot(studio, frame, options);
}

} // namespace pluss::pl::flash
