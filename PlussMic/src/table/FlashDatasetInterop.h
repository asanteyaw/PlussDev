#pragma once

#include "table/DataFrame.h"

#include "pluss/codeh/codeh.h"
#include "pluss/pl/flash.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::table {

using FlashRectifyPolicy = ::pluss::pl::flash::FramePolicy;
using FlashFromFrameOptions = ::pluss::pl::flash::FromFrameOptions;
using FlashToFrameOptions = ::pluss::pl::flash::ToFrameOptions;

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

} // namespace detail

[[nodiscard]] inline pluss::pl::flash::Dataset flash_dataset(
    const DataFrame& frame,
    const FlashFromFrameOptions& options = {}) {
  PLUSS_CHECK(frame.defined(), "flash::Dataset::from_frame: DataFrame is undefined");

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
                "flash::Dataset::from_frame: missing feature '" + name + "'");
    const auto column = frame.features(name);
    const auto mask = detail::compact_validity_mask_(column);

    switch (column.kind()) {
      case ThreadKind::Series:
        PLUSS_CHECK(options.include_series,
                    "flash::Dataset::from_frame: series feature '" + name + "' was selected but series export is disabled");
        numeric_names.push_back(name);
        out.numeric_values.push_back(column.as_series());
        out.numeric_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("numeric");
        break;
      case ThreadKind::Note:
        PLUSS_CHECK(options.include_text,
                    "flash::Dataset::from_frame: text feature '" + name + "' was selected but text export is disabled");
        text_names.push_back(name);
        out.text_values.push_back(column.as_note());
        out.text_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("text");
        break;
      case ThreadKind::Timeline:
        PLUSS_CHECK(options.include_timeline,
                    "flash::Dataset::from_frame: timeline feature '" + name + "' was selected but temporal export is disabled");
        temporal_names.push_back(name);
        out.temporal_values.push_back(column.as_timeline());
        out.temporal_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("temporal");
        break;
      case ThreadKind::TimeIndex:
        PLUSS_CHECK(options.include_timeindex_as_timeline,
                    "flash::Dataset::from_frame: timeindex feature '" + name + "' requires include_timeindex_as_timeline=true");
        temporal_names.push_back(name);
        out.temporal_values.push_back(column.as_timeindex().timeline());
        out.temporal_validity.push_back(mask);
        feature_order.push_back(name);
        feature_kinds.emplace_back("temporal");
        break;
      default:
        PLUSS_FAIL("flash::Dataset::from_frame: unsupported thread kind for feature '" + name + "'");
    }
  }

  PLUSS_CHECK(!feature_order.empty(), "flash::Dataset::from_frame: no exportable features were selected");

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
[[nodiscard]] inline Dataset Dataset::from_frame(
    const ::pluss::table::DataFrame& frame,
    const FromFrameOptions& options) {
  return ::pluss::table::flash_dataset(frame, options);
}

[[nodiscard]] inline ::pluss::table::DataFrame Dataset::frame(
    const ToFrameOptions& options) const {
  return ::pluss::table::frame_from_flash_dataset(*this, options);
}

} // namespace pluss::pl::flash
