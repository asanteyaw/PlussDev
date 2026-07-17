#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "pluss/codeh/timeindex_factory.h"
#include "pluss/coden/note_factory.h"

namespace pluss::table {
namespace {

using RowMap = std::vector<std::int64_t>;

struct CasePlan {
  RowMap left{};
  RowMap right{};
  Thread output_cases;
};

struct FeaturePlan {
  std::vector<std::string> names{};
  std::vector<std::int64_t> left{};
  std::vector<std::int64_t> right{};
};

std::vector<Thread> dataframe_columns(const DataFrame& frame) {
  std::vector<Thread> columns;
  const auto& names = frame.features();
  columns.reserve(names.size());
  for (const auto& name : names) {
    columns.push_back(frame.features(name));
  }
  return columns;
}

pluss::codem::Platform thread_platform(const Thread& thread) {
  switch (thread.kind()) {
    case ThreadKind::Series:
      return thread.as_series().platform();
    case ThreadKind::Timeline:
      return thread.as_timeline().epoch_nanos().platform();
    case ThreadKind::TimeIndex:
      return thread.as_timeindex().timeline().epoch_nanos().platform();
    case ThreadKind::Note:
      return pluss::codem::Platform::CPU;
  }
  PLUSS_FAIL("DataFrame::match: unsupported Thread kind");
}

pluss::Series bool_series_on(const std::vector<bool>& values,
                             const pluss::codem::Platform platform) {
  auto cpu = pluss::Series::zeros(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Bool,
      pluss::codem::Platform::CPU);
  auto* out = reinterpret_cast<bool*>(cpu.raw_data());
  std::copy(values.begin(), values.end(), out);
  return platform == pluss::codem::Platform::CPU ? cpu : cpu.to(platform);
}

std::vector<bool> thread_validity(const Thread& thread) {
  if (detail::has_validity_mask(thread)) {
    return detail::validity_mask(thread)->to_vec<bool>();
  }
  return std::vector<bool>(static_cast<std::size_t>(thread.size()), true);
}

template <class T>
void append_scalar_keys(const pluss::Series& series,
                        const std::vector<bool>& valid,
                        std::vector<std::string>& keys) {
  const auto values = series.to_vec<T>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!valid[i]) {
      keys[i] = std::string(1, '\0');
      continue;
    }
    if constexpr (std::is_floating_point_v<T>) {
      if (std::isnan(values[i])) {
        keys[i] = std::string(1, '\0');
        continue;
      }
      const T normalized = values[i] == static_cast<T>(0) ? static_cast<T>(0) : values[i];
      keys[i].assign(
          reinterpret_cast<const char*>(&normalized),
          sizeof(T));
    } else if constexpr (std::is_same_v<T, bool>) {
      keys[i].assign(1, values[i] ? '\1' : '\0');
    } else {
      keys[i].assign(
          reinterpret_cast<const char*>(&values[i]),
          sizeof(T));
    }
    keys[i].insert(keys[i].begin(), '\1');
  }
}

std::vector<std::string> case_keys(const Thread& cases) {
  const auto valid = thread_validity(cases);
  std::vector<std::string> keys(static_cast<std::size_t>(cases.size()));

  switch (cases.kind()) {
    case ThreadKind::Series: {
      const auto& series = cases.as_series();
      switch (series.scalar_type()) {
        case pluss::codem::ScalarType::Bool:
          append_scalar_keys<bool>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::UInt8:
          append_scalar_keys<std::uint8_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::UInt16:
          append_scalar_keys<std::uint16_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::UInt32:
          append_scalar_keys<std::uint32_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::UInt64:
          append_scalar_keys<std::uint64_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Int8:
          append_scalar_keys<std::int8_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Int16:
          append_scalar_keys<std::int16_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Int32:
          append_scalar_keys<std::int32_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Int64:
          append_scalar_keys<std::int64_t>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Float16:
        case pluss::codem::ScalarType::BFloat16:
        case pluss::codem::ScalarType::Float8E4M3FN:
        case pluss::codem::ScalarType::Float8E5M2:
          append_scalar_keys<float>(
              series.to(pluss::codem::ScalarType::Float32),
              valid,
              keys);
          break;
        case pluss::codem::ScalarType::Float32:
          append_scalar_keys<float>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Float64:
          append_scalar_keys<double>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Complex64:
          append_scalar_keys<pluss::codem::Complex64>(series, valid, keys);
          break;
        case pluss::codem::ScalarType::Complex128:
          append_scalar_keys<pluss::codem::Complex128>(series, valid, keys);
          break;
        default:
          PLUSS_FAIL(
              "DataFrame::match: unsupported numeric case dtype");
      }
      break;
    }
    case ThreadKind::Note: {
      const auto values = cases.as_note().vector();
      for (std::size_t i = 0; i < values.size(); ++i) {
        keys[i] = !valid[i] ? std::string(1, '\0')
                            : std::string("\2") + std::string(values[i]);
      }
      break;
    }
    case ThreadKind::Timeline:
    case ThreadKind::TimeIndex: {
      const auto ticks =
          cases.kind() == ThreadKind::Timeline
              ? cases.as_timeline().epoch_nanos().to_vec<std::int64_t>()
              : cases.as_timeindex().timeline().epoch_nanos().to_vec<std::int64_t>();
      for (std::size_t i = 0; i < ticks.size(); ++i) {
        if (!valid[i] || ticks[i] == pluss::Timeline::kNaT) {
          keys[i] = std::string(1, '\0');
          continue;
        }
        keys[i].assign(
            reinterpret_cast<const char*>(&ticks[i]),
            sizeof(std::int64_t));
        keys[i].insert(keys[i].begin(), '\3');
      }
      break;
    }
  }
  return keys;
}

void ensure_case_types_match(const Thread& left, const Thread& right);

void append_key_part_(std::string& out, const std::string& part) {
  const auto n = static_cast<std::uint64_t>(part.size());
  out.append(reinterpret_cast<const char*>(&n), sizeof(n));
  out.append(part);
}

std::vector<std::string> structured_case_keys(const DataFrame& frame) {
  const auto names = frame.label().names();
  PLUSS_CHECK(!names.empty(), "DataFrame::match: structured case axis has no levels");
  std::vector<std::vector<std::string>> level_keys;
  level_keys.reserve(names.size());
  for (const auto& name : names) {
    level_keys.push_back(case_keys(frame.label().part(name)));
  }

  std::vector<std::string> out(static_cast<std::size_t>(frame.ncases()));
  for (std::int64_t row = 0; row < frame.ncases(); ++row) {
    auto& key = out[static_cast<std::size_t>(row)];
    key.push_back('\4');
    for (const auto& keys : level_keys) {
      append_key_part_(key, keys[static_cast<std::size_t>(row)]);
    }
  }
  return out;
}

void ensure_structured_case_axes_match(const DataFrame& left, const DataFrame& right) {
  if (!left.label().has() || !right.label().has()) {
    return;
  }
  PLUSS_CHECK(
      left.label().names() == right.label().names(),
      "DataFrame::match: structured case levels differ; reorder/rename levels before aligning");
  const auto names = left.label().names();
  for (const auto& name : names) {
    ensure_case_types_match(left.label().part(name), right.label().part(name));
  }
}

std::vector<std::string> frame_case_keys(const DataFrame& frame) {
  return frame.label().has() ? structured_case_keys(frame)
                             : case_keys(frame.label().values());
}

void ensure_case_types_match(const Thread& left, const Thread& right) {
  PLUSS_CHECK(
      left.kind() == right.kind(),
      "DataFrame::match: case payload kinds differ; cast the case axes "
      "to a common kind first");
  if (left.kind() == ThreadKind::Series) {
    PLUSS_CHECK(
        left.as_series().scalar_type() == right.as_series().scalar_type(),
        "DataFrame::match: numeric case dtypes differ; cast the case "
        "axes to a common dtype first");
  }
}

std::unordered_map<std::string, RowMap> positions_by_key(
    const std::vector<std::string>& keys) {
  std::unordered_map<std::string, RowMap> out;
  out.reserve(keys.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(keys.size()); ++i) {
    out[keys[static_cast<std::size_t>(i)]].push_back(i);
  }
  return out;
}

void reject_repeated_caselbl(
    const std::unordered_map<std::string, RowMap>& positions,
    std::string_view side) {
  const auto repeated = std::find_if(
      positions.begin(),
      positions.end(),
      [](const auto& entry) { return entry.second.size() > 1; });
  PLUSS_CHECK(
      repeated == positions.end(),
      "DataFrame::match: repeated cases on the " + std::string(side) +
          " frame require RepeatedCaseRule::PairInOrder");
}

Thread take_cases(const Thread& cases, const RowMap& rows) {
  return cases.take(std::span<const std::int64_t>(rows.data(), rows.size()));
}

std::optional<pluss::Series> joined_validity(
    const Thread& left,
  const Thread& right,
  const pluss::codem::Platform platform) {
  if (!detail::has_validity_mask(left) && !detail::has_validity_mask(right)) {
    return std::nullopt;
  }
  auto left_mask = detail::has_validity_mask(left)
      ? *detail::validity_mask(left)
      : pluss::Series::ones({left.size()}, pluss::codem::ScalarType::Bool, platform);
  auto right_mask = detail::has_validity_mask(right)
      ? *detail::validity_mask(right)
      : pluss::Series::ones({right.size()}, pluss::codem::ScalarType::Bool, platform);
  if (left_mask.platform() != platform) {
    left_mask = left_mask.to(platform);
  }
  if (right_mask.platform() != platform) {
    right_mask = right_mask.to(platform);
  }
  std::vector<pluss::Series> tail{right_mask};
  return left_mask.unite(tail, 0);
}

Thread append_cases(const Thread& left, const Thread& right) {
  if (left.size() == 0) {
    return right;
  }
  if (right.size() == 0) {
    return left;
  }
  ensure_case_types_match(left, right);

  switch (left.kind()) {
    case ThreadKind::Series: {
      PLUSS_CHECK(
          left.as_series().platform() == right.as_series().platform(),
          "DataFrame::match: numeric case axes must use the same platform");
      std::vector<pluss::Series> tail{right.as_series()};
      auto values = left.as_series().unite(tail, 0);
      return Thread(
          values,
          joined_validity(left, right, values.platform()));
    }
    case ThreadKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(left.size() + right.size()));
      for (const auto value : left.as_note().vector()) {
        values.emplace_back(value);
      }
      for (const auto value : right.as_note().vector()) {
        values.emplace_back(value);
      }
      return Thread(
          pluss::make_note(values),
          joined_validity(left, right, pluss::codem::Platform::CPU));
    }
    case ThreadKind::Timeline: {
      auto values = left.as_timeline().concat(right.as_timeline());
      return Thread(
          values,
          joined_validity(left, right, values.epoch_nanos().platform()));
    }
    case ThreadKind::TimeIndex: {
      auto values = left.as_timeindex()
                        .timeline()
                        .concat(right.as_timeindex().timeline())
                        .to_timeindex(false);
      return Thread(
          values,
          joined_validity(
              left,
              right,
              values.timeline().epoch_nanos().platform()));
    }
  }
  PLUSS_FAIL("DataFrame::match: unsupported case payload kind");
}

Thread append_feature_values(const Thread& left, const Thread& right) {
  PLUSS_CHECK(
      left.kind() == right.kind(),
      "DataFrame::extend: matching features must use the same Thread kind");
  return left.append(right);
}

CasePlan plan_cases(
    const DataFrame& left,
    const DataFrame& right,
    const FrameMatchOptions& options) {
  ensure_structured_case_axes_match(left, right);
  if (!left.label().has() || !right.label().has()) {
    ensure_case_types_match(left.label().values(), right.label().values());
  }
  const auto left_keys = frame_case_keys(left);
  const auto right_keys = frame_case_keys(right);

  if (left_keys == right_keys) {
    RowMap identity(left.ncases());
    std::iota(identity.begin(), identity.end(), std::int64_t{0});
    return {identity, identity, left.label().values()};
  }

  const auto left_positions = positions_by_key(left_keys);
  const auto right_positions = positions_by_key(right_keys);
  if (options.repeated_cases == RepeatedCaseRule::Reject) {
    reject_repeated_caselbl(left_positions, "left");
    reject_repeated_caselbl(right_positions, "right");
  }

  CasePlan plan;
  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(std::max(left_keys.size(), right_keys.size()));

  const auto append_from_left = [&](const std::int64_t row) {
    const auto& key = left_keys[static_cast<std::size_t>(row)];
    const auto occurrence = seen[key]++;
    const auto found = right_positions.find(key);
    plan.left.push_back(row);
    plan.right.push_back(
        found != right_positions.end() && occurrence < found->second.size()
            ? found->second[occurrence]
            : -1);
  };
  const auto append_from_right = [&](const std::int64_t row) {
    const auto& key = right_keys[static_cast<std::size_t>(row)];
    const auto occurrence = seen[key]++;
    const auto found = left_positions.find(key);
    plan.right.push_back(row);
    plan.left.push_back(
        found != left_positions.end() && occurrence < found->second.size()
            ? found->second[occurrence]
            : -1);
  };

  switch (options.cases) {
    case FrameMatchRule::KeepLeft:
      for (std::int64_t row = 0; row < left.ncases(); ++row) {
        append_from_left(row);
      }
      plan.output_cases = left.label().values();
      break;
    case FrameMatchRule::KeepRight:
      for (std::int64_t row = 0; row < right.ncases(); ++row) {
        append_from_right(row);
      }
      plan.output_cases = right.label().values();
      break;
    case FrameMatchRule::Common: {
      RowMap output_rows;
      for (std::int64_t row = 0; row < left.ncases(); ++row) {
        const auto& key = left_keys[static_cast<std::size_t>(row)];
        const auto occurrence = seen[key]++;
        const auto found = right_positions.find(key);
        if (found == right_positions.end() || occurrence >= found->second.size()) {
          continue;
        }
        plan.left.push_back(row);
        plan.right.push_back(found->second[occurrence]);
        output_rows.push_back(row);
      }
      plan.output_cases = take_cases(left.label().values(), output_rows);
      break;
    }
    case FrameMatchRule::Unite: {
      for (std::int64_t row = 0; row < left.ncases(); ++row) {
        append_from_left(row);
      }
      RowMap right_only;
      std::unordered_map<std::string, std::size_t> right_seen;
      right_seen.reserve(right_positions.size());
      for (std::int64_t row = 0; row < right.ncases(); ++row) {
        const auto& key = right_keys[static_cast<std::size_t>(row)];
        const auto occurrence = right_seen[key]++;
        const auto found = left_positions.find(key);
        if (found != left_positions.end() && occurrence < found->second.size()) {
          continue;
        }
        plan.left.push_back(-1);
        plan.right.push_back(row);
        right_only.push_back(row);
      }
      plan.output_cases = append_cases(
          left.label().values(),
          take_cases(right.label().values(), right_only));
      break;
    }
  }
  return plan;
}

FeaturePlan plan_features(
    const DataFrame& left,
    const DataFrame& right,
    const FrameMatchRule rule) {
  const auto& left_names = left.features();
  const auto& right_names = right.features();
  std::unordered_map<std::string, std::int64_t> left_position;
  std::unordered_map<std::string, std::int64_t> right_position;
  left_position.reserve(left_names.size());
  right_position.reserve(right_names.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(left_names.size()); ++i) {
    left_position.emplace(left_names[static_cast<std::size_t>(i)], i);
  }
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(right_names.size()); ++i) {
    right_position.emplace(right_names[static_cast<std::size_t>(i)], i);
  }

  FeaturePlan plan;
  const auto append = [&](const std::string& name) {
    plan.names.push_back(name);
    const auto left_found = left_position.find(name);
    const auto right_found = right_position.find(name);
    plan.left.push_back(left_found == left_position.end() ? -1 : left_found->second);
    plan.right.push_back(right_found == right_position.end() ? -1 : right_found->second);
  };

  if (rule == FrameMatchRule::KeepRight) {
    for (const auto& name : right_names) {
      append(name);
    }
    return plan;
  }

  for (const auto& name : left_names) {
    if (rule != FrameMatchRule::Common || right_position.contains(name)) {
      append(name);
    }
  }
  if (rule == FrameMatchRule::Unite) {
    for (const auto& name : right_names) {
      if (!left_position.contains(name)) {
        append(name);
      }
    }
  }
  return plan;
}

Thread null_thread_like(const Thread& reference, const std::int64_t size) {
  PLUSS_CHECK(size >= 0, "DataFrame::match: invalid null feature size");
  Thread values;
  if (reference.size() > 0) {
    RowMap zeros(static_cast<std::size_t>(size), 0);
    values = reference.take(
        std::span<const std::int64_t>(zeros.data(), zeros.size()));
  } else {
    switch (reference.kind()) {
      case ThreadKind::Series:
        values = Thread(pluss::Series::zeros(
            {size},
            reference.as_series().scalar_type(),
            reference.as_series().platform()));
        break;
      case ThreadKind::Note:
        values = Thread(pluss::make_note(
            std::vector<std::string>(static_cast<std::size_t>(size))));
        break;
      case ThreadKind::Timeline: {
        auto ticks = pluss::Series::zeros(
            {size},
            pluss::codem::ScalarType::Int64,
            reference.as_timeline().epoch_nanos().platform());
        values = Thread(pluss::Timeline::fromSeries(ticks));
        break;
      }
      case ThreadKind::TimeIndex: {
        auto ticks = pluss::Series::zeros(
            {size},
            pluss::codem::ScalarType::Int64,
            reference.as_timeindex().timeline().epoch_nanos().platform());
        values = Thread(
            pluss::Timeline::fromSeries(ticks).to_timeindex(false));
        break;
      }
    }
  }
  return detail::with_validity_mask(values, bool_series_on(
      std::vector<bool>(static_cast<std::size_t>(size), false),
      thread_platform(values)));
}

Thread remap_thread(const Thread& source, const RowMap& rows) {
  if (rows.empty()) {
    return source.take(std::span<const std::int64_t>{});
  }
  if (source.size() == 0) {
    return null_thread_like(source, static_cast<std::int64_t>(rows.size()));
  }

  RowMap safe(rows.size());
  std::vector<bool> present(rows.size());
  std::transform(
      rows.begin(),
      rows.end(),
      safe.begin(),
      [](const std::int64_t row) { return row < 0 ? 0 : row; });
  std::transform(
      rows.begin(),
      rows.end(),
      present.begin(),
      [](const std::int64_t row) { return row >= 0; });

  auto out = source.take(
      std::span<const std::int64_t>(safe.data(), safe.size()));
  if (std::all_of(present.begin(), present.end(), [](const bool value) {
        return value;
      })) {
    return out;
  }

  const auto platform = thread_platform(out);
  auto mask = bool_series_on(present, platform);
  if (detail::has_validity_mask(out)) {
    auto prior = *detail::validity_mask(out);
    if (prior.platform() != platform) {
      prior = prior.to(platform);
    }
    mask = prior.logical_and(mask);
  }
  return detail::with_validity_mask(out, mask);
}

DataFrame remap_frame(
    const DataFrame& source,
    const DataFrame& peer,
    const RowMap& rows,
    const FeaturePlan& features,
    const bool left_side,
    const Thread& output_cases) {
  const auto source_columns = dataframe_columns(source);
  const auto peer_columns = dataframe_columns(peer);
  const auto& source_positions = left_side ? features.left : features.right;
  const auto& peer_positions = left_side ? features.right : features.left;

  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(features.names.size());
  for (std::size_t i = 0; i < features.names.size(); ++i) {
    const auto source_position = source_positions[i];
    if (source_position >= 0) {
      output.emplace_back(
          features.names[i],
          remap_thread(
              source_columns[static_cast<std::size_t>(source_position)],
              rows));
      continue;
    }
    const auto peer_position = peer_positions[i];
    PLUSS_CHECK(
        peer_position >= 0,
        "DataFrame::match: internal feature mapping failure");
    output.emplace_back(
        features.names[i],
        null_thread_like(
            peer_columns[static_cast<std::size_t>(peer_position)],
            static_cast<std::int64_t>(rows.size())));
  }
  return DataFrame(std::move(output), output_cases).title(source.title());
}

} // namespace

FrameMatch DataFrame::match(
    const DataFrame& other,
    const FrameMatchOptions& options) const {
  PLUSS_CHECK(defined(), "DataFrame::match: left frame is undefined");
  PLUSS_CHECK(other.defined(), "DataFrame::match: right frame is undefined");

  if (options.repeated_cases == RepeatedCaseRule::PairInOrder &&
      case_labels_().shares_payload_with(other.label().values()) &&
      features() == other.features()) {
    return FrameMatch{*this, other};
  }

  auto cases_plan = plan_cases(*this, other, options);
  const auto features_plan =
      plan_features(*this, other, options.features);

  FrameMatch result;
  result.left = remap_frame(
      *this,
      other,
      cases_plan.left,
      features_plan,
      true,
      cases_plan.output_cases);
  result.right = remap_frame(
      other,
      *this,
      cases_plan.right,
      features_plan,
      false,
      cases_plan.output_cases);
  return result;
}

DataFrame DataFrame::extend(
    const DataFrame& other,
    const FrameMatchRule feature_rule,
    const bool fresh_cases,
    const bool reject_repeats) const {
  PLUSS_CHECK(defined(), "DataFrame::extend: left frame is undefined");
  PLUSS_CHECK(other.defined(), "DataFrame::extend: right frame is undefined");

  const auto feature_plan = plan_features(*this, other, feature_rule);
  const auto left_columns = dataframe_columns(*this);
  const auto right_columns = dataframe_columns(other);
  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(feature_plan.names.size());

  for (std::size_t i = 0; i < feature_plan.names.size(); ++i) {
    const auto left_position = feature_plan.left[i];
    const auto right_position = feature_plan.right[i];
    Thread left_values;
    Thread right_values;
    if (left_position >= 0) {
      left_values = left_columns[static_cast<std::size_t>(left_position)];
    } else {
      right_values = right_columns[static_cast<std::size_t>(right_position)];
      left_values = null_thread_like(right_values, ncases());
    }
    if (right_position >= 0) {
      right_values = right_columns[static_cast<std::size_t>(right_position)];
    } else {
      right_values = null_thread_like(left_values, other.ncases());
    }
    output.emplace_back(
        feature_plan.names[i],
        append_feature_values(left_values, right_values));
  }

  Thread output_cases;
  if (fresh_cases) {
    output_cases = Thread(pluss::Series::arange(
        0.0,
        static_cast<double>(ncases() + other.ncases()),
        1.0,
        pluss::codem::ScalarType::Int64,
        pluss::codem::Platform::CPU));
  } else {
    output_cases = append_cases(case_labels_(), other.label().values());
    if (reject_repeats) {
      const auto positions = positions_by_key(case_keys(output_cases));
      reject_repeated_caselbl(positions, "combined");
    }
  }
  return DataFrame(std::move(output), output_cases).title(title());
}

} // namespace pluss::table
