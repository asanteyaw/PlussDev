#include "table/DataFrame.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "pluss/coden/note_factory.h"
#include "table/display/Display.h"

namespace pluss::table {

namespace {

std::string unique_feature_name(std::string candidate,
                                std::unordered_set<std::string>& seen,
                                std::int64_t row_index) {
  if (candidate.empty()) {
    candidate = fmt::format("cs_{}", row_index);
  }
  if (seen.insert(candidate).second) {
    return candidate;
  }

  auto base = candidate;
  std::int64_t suffix = 1;
  while (true) {
    auto probe = fmt::format("{}_{}", base, suffix++);
    if (seen.insert(probe).second) {
      return probe;
    }
  }
}

pluss::Note note_from_strings(const std::vector<std::string>& values) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(values.size())};
  return pluss::make_note(values, shape);
}

pluss::codem::Platform reshape_thread_platform(const Thread& thread) {
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
  PLUSS_FAIL("DataFrame::lengthen: unsupported Thread kind");
}

pluss::Series reshape_validity(
    const std::vector<Thread>& blocks,
    const pluss::codem::Platform platform) {
  std::vector<pluss::Series> masks;
  masks.reserve(blocks.size());
  for (const auto& block : blocks) {
    auto mask = detail::has_validity_mask(block)
        ? *detail::validity_mask(block)
        : pluss::Series::ones(
              {block.size()},
              pluss::codem::ScalarType::Bool,
              platform);
    if (mask.platform() != platform) {
      mask = mask.to(platform);
    }
    masks.push_back(std::move(mask));
  }
  std::vector<pluss::Series> tail(masks.begin() + 1, masks.end());
  return masks.front().unite(tail, 0);
}

Thread concatenate_reshape_threads(const std::vector<Thread>& blocks) {
  PLUSS_CHECK(!blocks.empty(), "DataFrame::lengthen: no value features selected");
  const auto kind = blocks.front().kind();
  const auto platform = reshape_thread_platform(blocks.front());
  const bool any_missing = std::any_of(
      blocks.begin(),
      blocks.end(),
      [](const Thread& block) { return detail::has_validity_mask(block); });
  for (const auto& block : blocks) {
    PLUSS_CHECK(
        block.kind() == kind,
        "DataFrame::lengthen: value features must use the same Thread kind");
    PLUSS_CHECK(
        reshape_thread_platform(block) == platform,
        "DataFrame::lengthen: value features must use the same platform");
  }

  Thread output;
  switch (kind) {
    case ThreadKind::Series: {
      const auto dtype = blocks.front().as_series().scalar_type();
      std::vector<pluss::Series> tail;
      tail.reserve(blocks.size() - 1);
      for (std::size_t i = 1; i < blocks.size(); ++i) {
        PLUSS_CHECK(
            blocks[i].as_series().scalar_type() == dtype,
            "DataFrame::lengthen: numeric value features must use the same dtype");
        tail.push_back(blocks[i].as_series());
      }
      output = Thread(blocks.front().as_series().unite(tail, 0));
      break;
    }
    case ThreadKind::Note: {
      std::vector<std::string> values;
      values.reserve(static_cast<std::size_t>(
          blocks.front().size() * static_cast<std::int64_t>(blocks.size())));
      for (const auto& block : blocks) {
        for (const auto value : block.as_note().vector()) {
          values.emplace_back(value);
        }
      }
      output = Thread(pluss::make_note(values));
      break;
    }
    case ThreadKind::Timeline: {
      auto values = blocks.front().as_timeline();
      for (std::size_t i = 1; i < blocks.size(); ++i) {
        values = values.concat(blocks[i].as_timeline());
      }
      output = Thread(values);
      break;
    }
    case ThreadKind::TimeIndex: {
      auto values = blocks.front().as_timeindex().timeline();
      for (std::size_t i = 1; i < blocks.size(); ++i) {
        values = values.concat(blocks[i].as_timeindex().timeline());
      }
      output = Thread(values.to_timeindex(false));
      break;
    }
  }
  return any_missing
      ? detail::with_validity_mask(output, reshape_validity(blocks, platform))
      : output;
}

std::string normalize_case_label(std::string raw) {
  if (raw.empty()) return raw;
  char* end = nullptr;
  const auto parsed = std::strtod(raw.c_str(), &end);
  if (end != nullptr && *end == '\0' && std::isfinite(parsed) && std::floor(parsed) == parsed) {
    return fmt::format("{:.0f}", parsed);
  }
  return raw;
}

std::string case_label_from_axis(const Thread& axis, std::int64_t row) {
  if (!detail::valid_at(axis, row)) {
    return {};
  }
  switch (axis.kind()) {
    case ThreadKind::Series: {
      const auto& s = axis.as_series();
      switch (s.scalar_type()) {
        case pluss::codem::ScalarType::Bool:
          return s.to_vec<bool>()[static_cast<std::size_t>(row)] ? "true" : "false";
        case pluss::codem::ScalarType::Int8:
        case pluss::codem::ScalarType::Int16:
        case pluss::codem::ScalarType::Int32:
        case pluss::codem::ScalarType::Int64:
          return fmt::format("{}", s.to_vec<std::int64_t>()[static_cast<std::size_t>(row)]);
        case pluss::codem::ScalarType::UInt8:
        case pluss::codem::ScalarType::UInt16:
        case pluss::codem::ScalarType::UInt32:
        case pluss::codem::ScalarType::UInt64:
          return fmt::format("{}", s.to_vec<std::uint64_t>()[static_cast<std::size_t>(row)]);
        case pluss::codem::ScalarType::Float16:
        case pluss::codem::ScalarType::BFloat16:
        case pluss::codem::ScalarType::Float32:
        case pluss::codem::ScalarType::Float64: {
          const auto value = s.to_vec<double>()[static_cast<std::size_t>(row)];
          if (std::isfinite(value) && std::floor(value) == value) {
            return fmt::format("{:.0f}", value);
          }
          return fmt::format("{}", value);
        }
        default:
          return std::string(axis.cast().note().as_note().at(row));
      }
    }
    case ThreadKind::Note:
      return normalize_case_label(std::string(axis.as_note().at(row)));
    case ThreadKind::Timeline:
      return fmt::format("{}", axis.as_timeline().ticks()[static_cast<std::size_t>(row)]);
    case ThreadKind::TimeIndex:
      return fmt::format("{}",
                         axis.as_timeindex().timeline().ticks()[static_cast<std::size_t>(row)]);
  }
  return {};
}

Thread null_singleton_like(const Thread& reference) {
  PLUSS_CHECK(reference.size() > 0, "DataFrame::unstack: cannot infer null payload from an empty feature");
  auto out = reference.take(pluss::msharp::RangeView::Int(std::initializer_list<std::int64_t>{0}));
  pluss::codem::Platform platform = pluss::codem::Platform::CPU;
  if (reference.kind() == ThreadKind::Series) {
    platform = reference.as_series().platform();
  } else if (reference.kind() == ThreadKind::Timeline) {
    platform = reference.as_timeline().epoch_nanos().platform();
  } else if (reference.kind() == ThreadKind::TimeIndex) {
    platform = reference.as_timeindex().timeline().epoch_nanos().platform();
  }
  return detail::with_validity_mask(out, pluss::Series::zeros(
      {1}, pluss::codem::ScalarType::Bool, platform));
}

std::string axis_value_key(const Thread& values, std::int64_t row) {
  return detail::valid_at(values, row) ? case_label_from_axis(values, row) : std::string("NULL");
}

} // namespace

DataFrameView DataFrame::transpose(bool render) const {
  const auto rows = ncases();
  const auto cols = nfeatures();

  std::vector<Thread> src_columns;
  src_columns.reserve(features().size());
  for (const auto& name : features()) {
    src_columns.push_back(this->features(name));
  }
  std::vector<pluss::Note> src_notes;
  src_notes.reserve(src_columns.size());
  for (const auto& col : src_columns) {
    src_notes.push_back(col.cast().note().as_note());
  }

  std::unordered_set<std::string> used_names;
  used_names.reserve(static_cast<std::size_t>(std::max<std::int64_t>(8, rows * 2)));

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(static_cast<std::size_t>(rows));

  for (std::int64_t r = 0; r < rows; ++r) {
    std::string feature_name;
    feature_name = normalize_case_label(case_label_from_axis(caselbl(), r));
    feature_name = unique_feature_name(std::move(feature_name), used_names, r);

    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(cols));
    for (std::int64_t c = 0; c < cols; ++c) {
      if (!detail::valid_at(src_columns[static_cast<std::size_t>(c)], r)) {
        values.emplace_back("NULL");
      } else {
        values.emplace_back(src_notes[static_cast<std::size_t>(c)].at(r));
      }
    }

    out_cols.emplace_back(std::move(feature_name), Thread(note_from_strings(values)));
  }

  Thread out_cases(note_from_strings(features()));
  auto out = DataFrame(std::move(out_cols), std::move(out_cases));
  return DataFrameView(std::move(out), render, "transpose", title());
}

DataFrame DataFrame::lengthen(
    pluss::msharp::RangeView::Of<std::string_view> id_features,
    pluss::msharp::RangeView::Of<std::string_view> value_features,
    std::string feature_name,
    std::string value_name,
    const bool fresh_cases) const {
  PLUSS_CHECK(defined(), "DataFrame::lengthen: frame is undefined");
  PLUSS_CHECK(!feature_name.empty(), "DataFrame::lengthen: feature_name cannot be empty");
  PLUSS_CHECK(!value_name.empty(), "DataFrame::lengthen: value_name cannot be empty");
  PLUSS_CHECK(
      feature_name != value_name,
      "DataFrame::lengthen: feature_name and value_name must differ");

  std::vector<std::string> ids;
  ids.reserve(id_features.size());
  std::unordered_set<std::string> selected;
  selected.reserve(features().size() + 2);
  for (const auto id : id_features) {
    const std::string name(id);
    PLUSS_CHECK(has_feature(name), "DataFrame::lengthen: unknown id feature '" + name + "'");
    PLUSS_CHECK(selected.insert(name).second,
                "DataFrame::lengthen: duplicate id feature '" + name + "'");
    ids.push_back(name);
  }

  std::vector<std::string> values;
  values.reserve(value_features.size());
  if (value_features.empty()) {
    for (const auto& name : features()) {
      if (!selected.contains(name)) {
        values.push_back(name);
      }
    }
  } else {
    for (const auto value : value_features) {
      const std::string name(value);
      PLUSS_CHECK(has_feature(name), "DataFrame::lengthen: unknown value feature '" + name + "'");
      PLUSS_CHECK(!selected.contains(name),
                  "DataFrame::lengthen: a feature cannot be both id and value: '" + name + "'");
      PLUSS_CHECK(
          std::find(values.begin(), values.end(), name) == values.end(),
          "DataFrame::lengthen: duplicate value feature '" + name + "'");
      values.push_back(name);
    }
  }
  PLUSS_CHECK(!values.empty(), "DataFrame::lengthen: no value features selected");
  PLUSS_CHECK(!selected.contains(feature_name) && !selected.contains(value_name),
              "DataFrame::lengthen: output names collide with id features");

  const auto block_count = static_cast<std::int64_t>(values.size());
  std::vector<std::int64_t> tiled_rows(
      static_cast<std::size_t>(ncases() * block_count));
  for (std::int64_t block = 0; block < block_count; ++block) {
    std::iota(
        tiled_rows.begin() + block * ncases(),
        tiled_rows.begin() + (block + 1) * ncases(),
        std::int64_t{0});
  }

  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(ids.size() + 2);
  for (const auto& id : ids) {
    output.emplace_back(id, features(id).take(tiled_rows));
  }

  std::vector<std::string> feature_values;
  feature_values.reserve(tiled_rows.size());
  for (const auto& name : values) {
    feature_values.insert(
        feature_values.end(),
        static_cast<std::size_t>(ncases()),
        name);
  }
  output.emplace_back(feature_name, Thread(pluss::make_note(feature_values)));

  std::vector<Thread> value_blocks;
  value_blocks.reserve(values.size());
  for (const auto& name : values) {
    value_blocks.push_back(features(name));
  }
  output.emplace_back(value_name, concatenate_reshape_threads(value_blocks));

  Thread output_cases = fresh_cases
      ? Thread(pluss::Series::arange(
            0.0,
            static_cast<double>(tiled_rows.size()),
            1.0,
            pluss::codem::ScalarType::Int64,
            pluss::codem::Platform::CPU))
      : caselbl().take(tiled_rows);
  return DataFrame(std::move(output), output_cases).title(title());
}

DataFrame DataFrame::stack(std::string feature_name,
                           std::string value_name) const {
  PLUSS_CHECK(defined(), "DataFrame::stack: frame is undefined");
  PLUSS_CHECK(!feature_name.empty(), "DataFrame::stack: feature_name cannot be empty");
  PLUSS_CHECK(!value_name.empty(), "DataFrame::stack: value_name cannot be empty");
  PLUSS_CHECK(feature_name != value_name,
              "DataFrame::stack: feature_name and value_name must differ");
  PLUSS_CHECK(nfeatures() > 0, "DataFrame::stack: expected at least one feature");

  std::vector<std::string> case_level_names;
  std::vector<Thread> case_level_values;
  if (has_caselbls()) {
    case_level_names = caselbls();
    case_level_values.reserve(case_level_names.size());
    for (const auto& name : case_level_names) {
      case_level_values.push_back(caselbl(name));
    }
  } else {
    case_level_names.push_back("case");
    case_level_values.push_back(caselbl());
  }

  std::unordered_set<std::string> output_names;
  output_names.reserve(case_level_names.size() + 2);
  for (const auto& name : case_level_names) {
    PLUSS_CHECK(output_names.insert(name).second,
                "DataFrame::stack: duplicate case level name '" + name + "'");
  }
  PLUSS_CHECK(output_names.insert(feature_name).second,
              "DataFrame::stack: feature_name collides with a case level");
  PLUSS_CHECK(output_names.insert(value_name).second,
              "DataFrame::stack: value_name collides with a case level or feature-name output");

  const auto block_count = nfeatures();
  std::vector<std::int64_t> tiled_rows(
      static_cast<std::size_t>(ncases() * block_count));
  for (std::int64_t block = 0; block < block_count; ++block) {
    std::iota(
        tiled_rows.begin() + block * ncases(),
        tiled_rows.begin() + (block + 1) * ncases(),
        std::int64_t{0});
  }

  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(case_level_names.size() + 2);
  for (std::size_t i = 0; i < case_level_names.size(); ++i) {
    output.emplace_back(case_level_names[i], case_level_values[i].take(tiled_rows));
  }

  std::vector<std::string> feature_values;
  feature_values.reserve(tiled_rows.size());
  for (const auto& name : features()) {
    feature_values.insert(
        feature_values.end(),
        static_cast<std::size_t>(ncases()),
        name);
  }
  output.emplace_back(feature_name, Thread(pluss::make_note(feature_values)));

  std::vector<Thread> value_blocks;
  value_blocks.reserve(features().size());
  for (const auto& name : features()) {
    value_blocks.push_back(this->features(name));
  }
  output.emplace_back(value_name, concatenate_reshape_threads(value_blocks));

  auto out = DataFrame(std::move(output));
  std::vector<std::string> case_features = case_level_names;
  case_features.push_back(feature_name);
  SetLabelOptions options;
  options.features = std::move(case_features);
  options.drop = true;
  options.verify = false;
  options.sort = false;
  return out.label().set(options).title(title());
}

DataFrame DataFrame::unstack(const UnstackOptions& options) const {
  PLUSS_CHECK(defined(), "DataFrame::unstack: frame is undefined");
  PLUSS_CHECK(has_caselbls(), "DataFrame::unstack: current DataFrame has no case labels");
  PLUSS_CHECK(!options.label.empty(), "DataFrame::unstack: label cannot be empty");
  PLUSS_CHECK(ncases() > 0, "DataFrame::unstack: cannot unstack an empty frame");

  const auto all_levels = caselbls();
  PLUSS_CHECK(std::find(all_levels.begin(), all_levels.end(), options.label) != all_levels.end(),
              "DataFrame::unstack: unknown case label '" + options.label + "'");

  std::vector<std::string> remaining_levels;
  remaining_levels.reserve(all_levels.size());
  for (const auto& name : all_levels) {
    if (name != options.label) {
      remaining_levels.push_back(name);
    }
  }

  const auto pivot_values = caselbl(options.label);
  std::vector<Thread> remaining_values;
  remaining_values.reserve(remaining_levels.size());
  for (const auto& name : remaining_levels) {
    remaining_values.push_back(caselbl(name));
  }

  std::vector<std::string> row_keys;
  std::vector<std::int64_t> first_rows;
  std::unordered_map<std::string, std::size_t> row_index;
  std::vector<std::string> column_keys;
  std::unordered_map<std::string, std::size_t> column_index;
  std::unordered_map<std::string, std::int64_t> cell_row;
  row_index.reserve(static_cast<std::size_t>(ncases()));
  column_index.reserve(static_cast<std::size_t>(ncases()));
  cell_row.reserve(static_cast<std::size_t>(ncases()));

  for (std::int64_t row = 0; row < ncases(); ++row) {
    std::string row_key;
    if (remaining_values.empty()) {
      row_key = "__all__";
    } else {
      for (const auto& values : remaining_values) {
        row_key += axis_value_key(values, row);
        row_key.append("\x1E", 1);
      }
    }
    const auto [row_it, row_inserted] = row_index.emplace(row_key, row_keys.size());
    if (row_inserted) {
      row_keys.push_back(row_key);
      first_rows.push_back(row);
    }

    const auto col_key = axis_value_key(pivot_values, row);
    const auto [col_it, col_inserted] = column_index.emplace(col_key, column_keys.size());
    if (col_inserted) {
      column_keys.push_back(col_key);
    }

    const auto cell_key = fmt::format("{}\x1F{}", row_it->second, col_it->second);
    PLUSS_CHECK(cell_row.emplace(cell_key, row).second,
                "DataFrame::unstack: duplicate cells found; aggregate before unstacking");
  }

  if (options.sort) {
    std::sort(column_keys.begin(), column_keys.end());
  }

  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(remaining_levels.size() + features().size() * column_keys.size());
  for (std::size_t i = 0; i < remaining_levels.size(); ++i) {
    output.emplace_back(remaining_levels[i], remaining_values[i].take(first_rows));
  }

  const bool multi_value = nfeatures() > 1;
  std::unordered_set<std::string> out_names;
  for (const auto& level_name : remaining_levels) {
    out_names.insert(level_name);
  }

  for (const auto& value_feature : features()) {
    const auto source = features(value_feature);
    for (std::size_t col = 0; col < column_keys.size(); ++col) {
      std::vector<Thread> blocks;
      blocks.reserve(row_keys.size());
      for (std::size_t row = 0; row < row_keys.size(); ++row) {
        const auto key = fmt::format("{}\x1F{}", row, col);
        const auto found = cell_row.find(key);
        blocks.push_back(found == cell_row.end()
                             ? null_singleton_like(source)
                             : source.take(pluss::msharp::RangeView::Int(
                                   std::initializer_list<std::int64_t>{found->second})));
      }
      auto out_name = multi_value
          ? value_feature + options.separator + column_keys[col]
          : column_keys[col];
      if (out_name.empty()) {
        out_name = multi_value ? value_feature : options.label;
      }
      PLUSS_CHECK(out_names.insert(out_name).second,
                  "DataFrame::unstack: duplicate output feature '" + out_name + "'");
      output.emplace_back(std::move(out_name), concatenate_reshape_threads(blocks));
    }
  }

  auto out = DataFrame(std::move(output));
  if (!remaining_levels.empty()) {
    SetLabelOptions set_options;
    set_options.features = remaining_levels;
    set_options.drop = options.drop;
    set_options.verify = false;
    set_options.sort = false;
    out = out.label().set(set_options);
  }
  return out.title(title());
}

} // namespace pluss::table
