TsGroup::TsGroup(Thread key) {
  if (key.defined()) {
    names.push_back(key.has_display_name() ? key.display_name() : std::string("group"));
    keys.push_back(std::move(key));
  }
}

TsGroup::TsGroup(const DataFrame& frame) {
  for (const auto& name : frame.features()) {
    auto key = frame.features(name);
    if (key.defined()) {
      keys.push_back(std::move(key));
      names.push_back(name);
    }
  }
}

bool TsGroup::defined() const noexcept {
  return !keys.empty();
}

std::int64_t TsGroup::size() const {
  if (keys.empty()) {
    return 0;
  }
  const auto n = keys.front().size();
  for (const auto& key : keys) {
    PLUSS_CHECK(key.defined(), "TsGroup::size: group key is undefined");
    PLUSS_CHECK(key.size() == n, "TsGroup::size: all group keys must have the same length");
  }
  return n;
}

namespace {

using pluss::ScalarType;
using pluss::Series;

std::vector<std::string> normalize_ts_features_(const DataFrame& frame,
                                                pluss::msharp::RangeView::Of<std::string_view> features,
                                                std::string_view where) {
  if (features.size() == 0) {
    const auto names = frame.features();
    PLUSS_CHECK(!names.empty(), std::string(where) + ": current frame has no features");
    std::vector<std::string> out;
    out.reserve(names.size());
    for (const auto& name : names) {
      const auto value = frame.features(name);
      // Auto-detection binds only the numeric Series columns as ts() value
      // features. Non-numeric columns (Note/Timeline/TimeIndex/Bool) are
      // silently excluded rather than rejected, since a column like this is
      // typically meant to serve as a `time_feature`-by-name axis argument
      // (see DataFrameTsAccessor::calendar/resample/asfreq/align_to), not as
      // a value to run time-series arithmetic over.
      if (value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series())) {
        out.push_back(name);
      }
    }
    PLUSS_CHECK(!out.empty(),
                std::string(where) + ": no features supplied; current frame contains no numeric Series "
                    "feature to bind. Select time-series value feature(s) first, e.g. df.features({\"price\"}).ts()");
    return out;
  }

  std::unordered_set<std::string> seen;
  std::vector<std::string> out;
  out.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(frame.get(name).has_value(), std::string(where) + ": unknown feature '" + name + "'");
    const auto value = frame.features(name);
    PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
                std::string(where) + ": feature '" + name + "' must be a numeric Series");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    out.push_back(name);
  }
  return out;
}

pluss::TimeIndex time_axis_to_index_(const Thread& axis, std::string_view where) {
  if (axis.kind() == ThreadKind::TimeIndex) {
    return axis.as_timeindex();
  }
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline,
              std::string(where) + ": axis feature must be Timeline/TimeIndex");
  return axis.as_timeline().to_timeindex(false);
}

pluss::TimeIndex require_time_axis_index_(const DataFrame& frame, std::string_view time_feature, std::string_view where) {
  PLUSS_CHECK(frame.get(time_feature).has_value(), std::string(where) + ": unknown time feature '" + std::string(time_feature) + "'");
  return time_axis_to_index_(frame.features(time_feature), where);
}

pluss::TimeIndex require_case_time_axis_index_(const DataFrame& frame, std::string_view where) {
  const auto axis = frame.label().values();
  PLUSS_CHECK(axis.defined(), std::string(where) + ": cases are undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": cases must be Timeline/TimeIndex. Use df.label().set(\"date\") first");
  return axis.kind() == ThreadKind::TimeIndex ? axis.as_timeindex() : axis.as_timeline().to_timeindex(false);
}

Thread require_time_axis_thread_(const Thread& axis, std::string_view where) {
  PLUSS_CHECK(axis.defined(), std::string(where) + ": target axis is undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": target axis must be Timeline or TimeIndex");
  return axis;
}

std::vector<Thread> cpu_group_keys_(const TsGroup& groups) {
  std::vector<Thread> out;
  out.reserve(groups.keys.size());
  for (const auto& key : groups.keys) {
    Thread cpu_key = key;
    if (key.kind() == ThreadKind::Series &&
        key.as_series().platform() != pluss::codem::Platform::CPU) {
      cpu_key = Thread(key.as_series().to(pluss::codem::Platform::CPU),
                       key.notnull().as_series().to(pluss::codem::Platform::CPU));
    }
    out.push_back(std::move(cpu_key));
  }
  return out;
}

std::string ts_group_key_at_(const std::vector<Thread>& cpu_groups, std::int64_t row) {
  std::string key;
  for (const auto& group : cpu_groups) {
    key.push_back('\2');
    if (group.is_valid(row)) {
      key.push_back('\1');
      key += thread_scalar_string_at(group, row);
    } else {
      key.push_back('\0');
    }
  }
  return key;
}

void validate_ts_options_(const DataFrame& frame, const TsOptions& options, std::string_view where) {
  if (options.axis.defined()) {
    PLUSS_CHECK(options.axis.size() == frame.ncases(),
                std::string(where) + ": axis length must match DataFrame cases");
    (void)require_time_axis_thread_(options.axis, where);
  }
  if (options.group.defined()) {
    PLUSS_CHECK(options.group.size() == frame.ncases(),
                std::string(where) + ": group length must match DataFrame cases");
  }
  if (options.anchor.defined()) {
    PLUSS_CHECK(options.anchor.size() == frame.ncases(),
                std::string(where) + ": anchor length must match DataFrame cases");
    (void)require_time_axis_thread_(options.anchor, where);
  }
}

Thread ts_source_axis_(const DataFrame& frame, const TsOptions& options, std::string_view where) {
  if (options.axis.defined()) {
    return require_time_axis_thread_(options.axis, where);
  }
  (void)require_case_time_axis_index_(frame, where);
  return frame.label().values();
}

pluss::TimeIndex thread_to_timeindex_(const Thread& axis, std::string_view where) {
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": axis must be Timeline or TimeIndex");
  return axis.kind() == ThreadKind::TimeIndex ? axis.as_timeindex() : axis.as_timeline().to_timeindex(false);
}

Thread resample_target_axis_(const Thread& source_axis,
                             const pluss::TimeIndex::ResampleOptions& options,
                             std::string_view where) {
  const auto source_ti = thread_to_timeindex_(source_axis, where);
  const auto bins = source_ti.resample_bins(options);
  return Thread(bins.labels);
}

pluss::Series ts_index_series_(const std::vector<std::int64_t>& values,
                               pluss::codem::Platform platform) {
  auto out = pluss::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Int64,
      pluss::codem::Platform::CPU);
  if (!values.empty()) {
    std::copy(values.begin(), values.end(),
              reinterpret_cast<std::int64_t*>(out.raw_data()));
  }
  return platform == pluss::codem::Platform::CPU ? out : out.to(platform);
}

Thread grouped_timed_rolling_(const Thread& values,
                              const Thread& axis,
                              const TsGroup& groups,
                              std::string_view duration,
                              WindowStatistic statistic,
                              const StatisticOptions& options,
                              WindowClosure closure,
                              std::string_view where) {
  PLUSS_CHECK(values.kind() == ThreadKind::Series && is_numeric_series_type(values.as_series()),
              std::string(where) + ": values must be numeric Series");
  PLUSS_CHECK(axis.size() == values.size(),
              std::string(where) + ": axis length must match values");
  PLUSS_CHECK(groups.defined(),
              std::string(where) + ": group keys are required");
  PLUSS_CHECK(groups.size() == values.size(),
              std::string(where) + ": group length must match values");

  const auto cpu_groups = cpu_group_keys_(groups);

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  rows_by_group.reserve(static_cast<std::size_t>(values.size()));
  for (std::int64_t row = 0; row < values.size(); ++row) {
    rows_by_group[ts_group_key_at_(cpu_groups, row)].push_back(row);
  }

  const auto platform = values.as_series().platform();
  const auto dtype = platform == pluss::codem::Platform::CPU
                         ? pluss::codem::ScalarType::Float64
                         : pluss::codem::ScalarType::Float32;
  auto output = pluss::Series::zeros({values.size()}, dtype, platform);
  auto output_valid = pluss::Series::zeros(
      {values.size()}, pluss::codem::ScalarType::Bool, platform);

  for (const auto& [key, rows] : rows_by_group) {
    (void)key;
    const auto index = ts_index_series_(rows, platform);
    const auto result = values.take(rows).ts().rolling(
        axis.take(rows), duration, statistic, options, closure);
    output = output.index_copy(0, index, result.as_series().to(dtype));
    output_valid = output_valid.index_copy(
        0, index, result.notnull().as_series().to(platform));
  }

  return Thread(output, output_valid).label().set(axis);
}

Thread grouped_fixed_rolling_(const Thread& values,
                              const TsGroup& groups,
                              std::int64_t window,
                              WindowStatistic statistic,
                              const WindowOptions& options,
                              std::string_view where) {
  PLUSS_CHECK(values.kind() == ThreadKind::Series && is_numeric_series_type(values.as_series()),
              std::string(where) + ": values must be numeric Series");
  PLUSS_CHECK(groups.defined(),
              std::string(where) + ": group keys are required");
  PLUSS_CHECK(groups.size() == values.size(),
              std::string(where) + ": group length must match values");

  const auto cpu_groups = cpu_group_keys_(groups);
  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  rows_by_group.reserve(static_cast<std::size_t>(values.size()));
  for (std::int64_t row = 0; row < values.size(); ++row) {
    rows_by_group[ts_group_key_at_(cpu_groups, row)].push_back(row);
  }

  const auto platform = values.as_series().platform();
  const auto dtype = platform == pluss::codem::Platform::CPU
                         ? pluss::codem::ScalarType::Float64
                         : pluss::codem::ScalarType::Float32;
  auto output = pluss::Series::zeros({values.size()}, dtype, platform);
  auto output_valid = pluss::Series::zeros(
      {values.size()}, pluss::codem::ScalarType::Bool, platform);

  for (const auto& [key, rows] : rows_by_group) {
    (void)key;
    const auto index = ts_index_series_(rows, platform);
    const auto result = values.take(rows).ts().rolling(window, statistic, options);
    output = output.index_copy(0, index, result.as_series().to(dtype));
    output_valid = output_valid.index_copy(
        0, index, result.notnull().as_series().to(platform));
  }

  return Thread(output, output_valid).label().set(values.label().values());
}

DataFrame make_frame_from_columns_(std::vector<std::pair<std::string, Thread>> cols, const Thread& cases) {
  return DataFrame(std::move(cols), cases);
}

std::string feature_suffix_(std::string_view feature, std::string_view tag, std::int64_t value) {
  return std::string(feature) + "_" + std::string(tag) + std::to_string(value);
}

std::string feature_suffix_(std::string_view feature, std::string_view tag) {
  return std::string(feature) + "_" + std::string(tag);
}

std::string_view resample_op_key_(pluss::subsp::stats::ts::ResampleOp op) {
  using ResampleOp = pluss::subsp::stats::ts::ResampleOp;
  switch (op) {
    case ResampleOp::Mean:
      return "mean";
    case ResampleOp::Sum:
      return "sum";
    case ResampleOp::Min:
      return "min";
    case ResampleOp::Max:
      return "max";
    case ResampleOp::First:
      return "first";
    case ResampleOp::Last:
      return "last";
    case ResampleOp::Count:
      return "count";
  }
  PLUSS_FAIL("DataFrameTsAccessor::resample: unsupported ResampleOp");
}

DataFrame ts_binary_frame_(const DataFrame& left,
                           const DataFrame& right,
                           const std::vector<std::string>& features,
                           const Thread& explicit_left_axis,
                           TsAlignMode mode,
                           int op) {
  const auto left_axis = explicit_left_axis.defined() ? explicit_left_axis : left.label().values();
  const auto right_axis = right.label().values();
  (void)time_axis_to_index_(left_axis, "DataFrameTsAccessor::binary");
  (void)time_axis_to_index_(right_axis, "DataFrameTsAccessor::binary");

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features.size());
  for (const auto& name : features) {
    PLUSS_CHECK(right.get(name).has_value(), "DataFrameTsAccessor::binary: right frame is missing feature '" + name + "'");
    const auto lhs = left.features(name);
    const auto rhs = right.features(name);
    switch (op) {
      case 0:
        out.emplace_back(name, lhs.ts().add(rhs, left_axis, right_axis, mode));
        break;
      case 1:
        out.emplace_back(name, lhs.ts().sub(rhs, left_axis, right_axis, mode));
        break;
      case 2:
        out.emplace_back(name, lhs.ts().mul(rhs, left_axis, right_axis, mode));
        break;
      case 3:
        out.emplace_back(name, lhs.ts().div(rhs, left_axis, right_axis, mode));
        break;
      default:
        PLUSS_FAIL("DataFrameTsAccessor: unsupported binary operation");
    }
  }
  return make_frame_from_columns_(std::move(out), left_axis);
}

Thread resample_target_axis_(const DataFrame& frame,
                             std::string_view time_feature,
                             std::string_view freq,
                             std::int64_t step,
                             std::string_view where) {
  const auto source_ti = time_feature.empty()
                             ? require_case_time_axis_index_(frame, where)
                             : require_time_axis_index_(frame, time_feature, where);
  const auto bins = source_ti.resample_bins(freq, step);
  return Thread(bins.labels);
}

Thread resample_target_axis_(const DataFrame& frame,
                             std::string_view time_feature,
                             const pluss::TimeIndex::ResampleOptions& options,
                             std::string_view where) {
  const auto source_ti = time_feature.empty()
                             ? require_case_time_axis_index_(frame, where)
                             : require_time_axis_index_(frame, time_feature, where);
  const auto bins = source_ti.resample_bins(options);
  return Thread(bins.labels);
}

std::int64_t ts_builder_duration_nanos_(std::string_view raw, std::string_view where) {
  auto first = raw.begin();
  auto last = raw.end();
  while (first != last && std::isspace(static_cast<unsigned char>(*first))) {
    ++first;
  }
  while (last != first && std::isspace(static_cast<unsigned char>(*(last - 1)))) {
    --last;
  }
  PLUSS_CHECK(first != last, std::string(where) + ": duration cannot be empty");

  std::int64_t value = 0;
  bool saw_digit = false;
  while (first != last && std::isdigit(static_cast<unsigned char>(*first))) {
    saw_digit = true;
    const auto digit = static_cast<std::int64_t>(*first - '0');
    PLUSS_CHECK(value <= (std::numeric_limits<std::int64_t>::max() - digit) / 10,
                std::string(where) + ": duration is too large");
    value = value * 10 + digit;
    ++first;
  }
  PLUSS_CHECK(saw_digit && value > 0,
              std::string(where) + ": duration must start with a positive integer");
  while (first != last && std::isspace(static_cast<unsigned char>(*first))) {
    ++first;
  }
  std::string unit(first, last);
  for (auto& ch : unit) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  PLUSS_CHECK(!unit.empty(),
              std::string(where) + ": duration unit is required, e.g. \"7D\", \"12H\", \"30min\"");

  std::int64_t multiplier = 0;
  if (unit == "ns" || unit == "nano" || unit == "nanos") {
    multiplier = 1;
  } else if (unit == "us" || unit == "micro" || unit == "micros") {
    multiplier = 1'000;
  } else if (unit == "ms" || unit == "milli" || unit == "millis") {
    multiplier = 1'000'000;
  } else if (unit == "s" || unit == "sec" || unit == "secs") {
    multiplier = 1'000'000'000;
  } else if (unit == "m" || unit == "min" || unit == "mins") {
    multiplier = 60LL * 1'000'000'000;
  } else if (unit == "h" || unit == "hr" || unit == "hrs") {
    multiplier = 60LL * 60LL * 1'000'000'000;
  } else if (unit == "d" || unit == "day" || unit == "days") {
    multiplier = 24LL * 60LL * 60LL * 1'000'000'000;
  } else if (unit == "w" || unit == "week" || unit == "weeks") {
    multiplier = 7LL * 24LL * 60LL * 60LL * 1'000'000'000;
  } else {
    PLUSS_FAIL(std::string(where) + ": unsupported duration unit '" + unit + "'");
  }
  PLUSS_CHECK(value <= std::numeric_limits<std::int64_t>::max() / multiplier,
              std::string(where) + ": duration is too large");
  return value * multiplier;
}

std::vector<std::string_view> ts_group_key_views_(
    const std::vector<std::string>& keys) {
  std::vector<std::string_view> out;
  out.reserve(keys.size());
  for (const auto& key : keys) {
    out.push_back(key);
  }
  return out;
}

enum class TsGroupedWindowAggOp : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Min = 2,
  Max = 3,
  Count = 4,
};

std::string grouped_window_output_name_(TsGroupedWindowAggOp op,
                                        std::string_view value_feature) {
  switch (op) {
    case TsGroupedWindowAggOp::Sum:
      return "sum_" + std::string(value_feature);
    case TsGroupedWindowAggOp::Mean:
      return "mean_" + std::string(value_feature);
    case TsGroupedWindowAggOp::Min:
      return "min_" + std::string(value_feature);
    case TsGroupedWindowAggOp::Max:
      return "max_" + std::string(value_feature);
    case TsGroupedWindowAggOp::Count:
      return "count_" + std::string(value_feature);
  }
  PLUSS_FAIL("DataFrameTsWindowAccessor: unsupported grouped window aggregate");
}

DataFrame::GroupOp grouped_window_group_op_(TsGroupedWindowAggOp op) {
  switch (op) {
    case TsGroupedWindowAggOp::Sum:
      return DataFrame::GroupOp::Sum;
    case TsGroupedWindowAggOp::Mean:
      return DataFrame::GroupOp::Mean;
    case TsGroupedWindowAggOp::Min:
      return DataFrame::GroupOp::Min;
    case TsGroupedWindowAggOp::Max:
      return DataFrame::GroupOp::Max;
    case TsGroupedWindowAggOp::Count:
      return DataFrame::GroupOp::Count;
  }
  PLUSS_FAIL("DataFrameTsWindowAccessor: unsupported grouped window aggregate");
}

std::vector<std::string> default_group_names_(std::size_t count) {
  std::vector<std::string> out;
  out.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    out.push_back(count == 1 ? "group" : "group_" + std::to_string(i));
  }
  return out;
}

struct GroupedWindowGroupColumns {
  DataFrame frame{};
  std::vector<std::string> internal_names{};
  std::vector<std::string> public_names{};
};

GroupedWindowGroupColumns attach_group_columns_(const DataFrame& frame,
                                                const TsGroup& groups,
                                                std::vector<std::string> public_names,
                                                std::string_view where) {
  PLUSS_CHECK(groups.defined(),
              std::string(where) + ": group keys are required");
  PLUSS_CHECK(groups.size() == frame.ncases(),
              std::string(where) + ": group length must match DataFrame cases");
  if (public_names.empty()) {
    public_names = default_group_names_(groups.keys.size());
  }
  PLUSS_CHECK(public_names.size() == groups.keys.size(),
              std::string(where) + ": group names length must match group key count");

  GroupedWindowGroupColumns out;
  out.frame = frame;
  out.public_names = std::move(public_names);
  out.internal_names.reserve(groups.keys.size());
  for (std::size_t i = 0; i < groups.keys.size(); ++i) {
    const auto internal = "__ts_group_" + std::to_string(i) + "__";
    out.frame = out.frame.set(internal, groups.keys[i]);
    out.internal_names.push_back(internal);
  }
  return out;
}

std::string resolve_grouped_window_value_feature_(
    const DataFrame& frame,
    const std::vector<std::string>& selected,
    std::string_view value_feature,
    bool numeric_required,
    std::string_view where) {
  std::string out;
  if (value_feature.empty()) {
    PLUSS_CHECK(selected.size() == 1,
                std::string(where) +
                    ": value feature is required unless exactly one ts feature is selected");
    out = selected.front();
  } else {
    out = owned_name(value_feature);
  }
  PLUSS_CHECK(frame.get(out).has_value(),
              std::string(where) + ": unknown value feature '" + out + "'");
  if (numeric_required) {
    const auto value = frame.features(out);
    PLUSS_CHECK(value.kind() == ThreadKind::Series &&
                    is_numeric_series_type(value.as_series()),
                std::string(where) + ": value feature '" + out + "' must be a numeric Series");
  }
  return out;
}

Thread grouped_window_axis_thread_(const Thread& axis,
                                   std::string_view where) {
  PLUSS_CHECK(axis.defined(),
              std::string(where) + ": time axis is undefined");
  if (axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex) {
    return axis.tl().epoch(TimeUnit::Nanos).cast(ThreadCastTarget::Series, ScalarType::Float64);
  }
  PLUSS_CHECK(axis.kind() == ThreadKind::Series &&
                  axis.as_series().scalar_type() == ScalarType::Int64,
              std::string(where) +
                  ": time axis must be Timeline, TimeIndex, or Int64 epoch-nanos Series");
  return axis.cast(ThreadCastTarget::Series, ScalarType::Float64);
}

DataFrame with_grouped_window_axis_(const DataFrame& frame,
                                    const Thread& axis,
                                    std::string_view output_feature,
                                    std::string_view where) {
  PLUSS_CHECK(axis.size() == frame.ncases(),
              std::string(where) + ": time axis length must match DataFrame cases");
  return frame.transform(
      output_feature,
      [axis, where_owned = std::string(where)](const DataFrame&) {
        return grouped_window_axis_thread_(axis, where_owned);
      });
}

Thread grouped_window_mask_(const Thread& axis,
                            const Thread& anchor,
                            std::int64_t duration_nanos,
                            WindowClosure closure) {
  const auto lower = anchor.sub(static_cast<double>(duration_nanos));
  switch (closure) {
    case WindowClosure::Right:
      return axis.gt(lower).land(axis.le(anchor));
    case WindowClosure::Left:
      return axis.ge(lower).land(axis.lt(anchor));
    case WindowClosure::Both:
      return axis.ge(lower).land(axis.le(anchor));
    case WindowClosure::Neither:
      return axis.gt(lower).land(axis.lt(anchor));
  }
  PLUSS_FAIL("DataFrameTsWindowAccessor: unsupported closure");
}

DataFrame grouped_window_aggregate_(const DataFrame& frame,
                                    const std::vector<std::string>& selected_features,
                                    const TsOptions& options,
                                    std::string_view duration,
                                    WindowClosure closure,
                                    TsGroupedWindowAggOp op,
                                    std::string_view value_feature,
                                    std::string_view where) {
  PLUSS_CHECK(options.group.defined(),
              std::string(where) + ": TsOptions.group is required");
  const auto value_name = resolve_grouped_window_value_feature_(
      frame,
      selected_features,
      value_feature,
      op != TsGroupedWindowAggOp::Count,
      where);
  const auto duration_nanos = ts_builder_duration_nanos_(duration, where);
  const auto axis = ts_source_axis_(frame, options, where);
  auto grouped_frame = attach_group_columns_(
      frame,
      options.group,
      options.group.names,
      where);
  const auto key_views = ts_group_key_views_(grouped_frame.internal_names);

  constexpr std::string_view time_axis_name = "__ts_window_time__";
  constexpr std::string_view anchor_axis_name = "__ts_window_anchor__";
  auto working = with_grouped_window_axis_(grouped_frame.frame, axis, time_axis_name, where);
  if (options.anchor.defined()) {
    working = with_grouped_window_axis_(working, options.anchor, anchor_axis_name, where);
  } else {
    const auto anchors = working.group_by(
        pluss::msharp::RangeView::Of<std::string_view>(key_views),
        {DataFrame::GroupSpec{
            std::string(anchor_axis_name),
            std::string(time_axis_name),
            DataFrame::GroupOp::Max,
        }},
        true,
        false);
    working = working.unify(pluss::msharp::RangeView::Of<std::string_view>(key_views),
                            anchors,
                            pluss::msharp::RangeView::Of<std::string_view>(key_views),
                            "keep_left",
                            "_anchor",
                            "many_to_one");
  }

  const auto mask = grouped_window_mask_(working.features(std::string(time_axis_name)),
                                         working.features(std::string(anchor_axis_name)),
                                         duration_nanos,
                                         closure);
  const auto filtered = working.filter(mask);
  const auto output_name = grouped_window_output_name_(op, value_name);
  DataFrame out = filtered.group_by(pluss::msharp::RangeView::Of<std::string_view>(key_views),
                                    {DataFrame::GroupSpec{
                                        output_name,
                                        value_name,
                                        grouped_window_group_op_(op),
                                    }},
                                    true,
                                    false);
  for (std::size_t i = 0; i < grouped_frame.internal_names.size(); ++i) {
    if (grouped_frame.internal_names[i] != grouped_frame.public_names[i]) {
      out = out.rename(grouped_frame.internal_names[i], grouped_frame.public_names[i]);
    }
  }
  return out;
}

} // namespace

DataFrameTsAccessor::DataFrameTsAccessor(DataFrame frame, std::vector<std::string> features, TsOptions options)
    : frame_(std::move(frame)), features_(std::move(features)), options_(std::move(options)) {
  PLUSS_CHECK(!features_.empty(), "DataFrameTsAccessor: expected at least one feature");
  validate_ts_options_(frame_, options_, "DataFrameTsAccessor");
}

DataFrameTsAccessor DataFrame::ts() const {
  return DataFrameTsAccessor(*this, normalize_ts_features_(*this, {}, "DataFrame::ts"));
}

DataFrameTsAccessor DataFrame::ts(const TsOptions& options) const {
  return DataFrameTsAccessor(*this, normalize_ts_features_(*this, {}, "DataFrame::ts"), options);
}

DataFrame DataFrameTsAccessor::returns(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().returns(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::log_returns(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().log_returns(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::diff(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().diff(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::lag(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lag(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::lead(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lead(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::rolling(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().rolling(window, statistic, options));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::rolling(
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  const auto axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::rolling");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    const auto values = frame_.features(name);
    if (options_.group.defined()) {
      out.emplace_back(name, grouped_timed_rolling_(
                                 values,
                                 axis,
                                 options_.group,
                                 duration,
                                 statistic,
                                 options,
                                 closure,
                                 "DataFrameTsAccessor::rolling"));
    } else {
      out.emplace_back(name, values.ts().rolling(axis, duration, statistic, options, closure));
    }
  }
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrame DataFrameTsAccessor::grouped(std::int64_t window,
                                       WindowStatistic statistic,
                                       const WindowOptions& options) const {
  PLUSS_CHECK(options_.group.defined(),
              "DataFrameTsAccessor::grouped: TsOptions.group is required. Use df.features(...).ts({.group = df.features(\"group\")})");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, grouped_fixed_rolling_(
                               frame_.features(name),
                               options_.group,
                               window,
                               statistic,
                               options,
                               "DataFrameTsAccessor::grouped"));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::grouped(std::string_view group_feature,
                                       std::int64_t window,
                                       WindowStatistic statistic,
                                       const WindowOptions& options) const {
  PLUSS_CHECK(frame_.get(group_feature).has_value(),
              "DataFrameTsAccessor::grouped: unknown group feature '" + std::string(group_feature) + "'");
  return grouped(frame_.features(group_feature), window, statistic, options);
}

DataFrame DataFrameTsAccessor::grouped(const Thread& groups,
                                       std::int64_t window,
                                       WindowStatistic statistic,
                                       const WindowOptions& options) const {
  PLUSS_CHECK(groups.defined(), "DataFrameTsAccessor::grouped: groups Thread is undefined");
  PLUSS_CHECK(groups.size() == frame_.ncases(),
              "DataFrameTsAccessor::grouped: groups length must match DataFrame cases");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().grouped(groups, window, statistic, options));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrameTsWindowAccessor DataFrameTsAccessor::window(std::string_view duration,
                                                      WindowClosure closure) const {
  return DataFrameTsWindowAccessor(frame_, features_, options_, owned_name(duration), closure);
}

DataFrame DataFrameTsAccessor::weigh(
    pluss::msharp::RangeView::Double weights,
    WindowStatistic statistic,
    std::int64_t min_count) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).weigh(weights, statistic, min_count));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::bounded(
    const WindowBoundsFn& bounds,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  PLUSS_CHECK(static_cast<bool>(bounds),
              "DataFrameTsAccessor::bounded: bounds callback is empty");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).bounded(bounds, statistic, options));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::window(
    pluss::msharp::RangeView::Of<DataFrame::WindowSpec> specs,
    bool keep_input) const {
  return frame_.window(specs, keep_input);
}

DataFrame DataFrameTsAccessor::rolling(
    pluss::msharp::RangeView::Of<DataFrame::WindowSpec> specs,
    bool keep_input) const {
  return window(specs, keep_input);
}

DataFrame DataFrameTsAccessor::expanding(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().expanding(statistic, options));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::ewm(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().ewm(
        alpha, statistic, adjusted, bias_corrected, min_count, ignore_nulls));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().zscore_rolling(window, min_periods, epsilon));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().fill_gaps(mode, constant));
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::sort(bool ascending, bool nulls_last) const {
  const auto axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::sort");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name));
  }
  return make_frame_from_columns_(std::move(out), axis).label().sort(ascending, nulls_last);
}

DataFrameTsWindowAccessor::DataFrameTsWindowAccessor(
    DataFrame frame,
    std::vector<std::string> features,
    TsOptions options,
    std::string duration,
    WindowClosure closure)
    : frame_(std::move(frame)),
      features_(std::move(features)),
      options_(std::move(options)),
      duration_(std::move(duration)),
      closure_(closure) {
  PLUSS_CHECK(!duration_.empty(),
              "DataFrameTsAccessor::window: duration cannot be empty");
  PLUSS_CHECK(options_.group.defined(),
              "DataFrameTsAccessor::window: TsOptions.group is required");
}

DataFrame DataFrameTsWindowAccessor::sum(std::string_view value_feature) const {
  return grouped_window_aggregate_(frame_,
                                   features_,
                                   options_,
                                   duration_,
                                   closure_,
                                   TsGroupedWindowAggOp::Sum,
                                   value_feature,
                                   "DataFrameTsWindowAccessor::sum");
}

DataFrame DataFrameTsWindowAccessor::mean(std::string_view value_feature) const {
  return grouped_window_aggregate_(frame_,
                                   features_,
                                   options_,
                                   duration_,
                                   closure_,
                                   TsGroupedWindowAggOp::Mean,
                                   value_feature,
                                   "DataFrameTsWindowAccessor::mean");
}

DataFrame DataFrameTsWindowAccessor::min(std::string_view value_feature) const {
  return grouped_window_aggregate_(frame_,
                                   features_,
                                   options_,
                                   duration_,
                                   closure_,
                                   TsGroupedWindowAggOp::Min,
                                   value_feature,
                                   "DataFrameTsWindowAccessor::min");
}

DataFrame DataFrameTsWindowAccessor::max(std::string_view value_feature) const {
  return grouped_window_aggregate_(frame_,
                                   features_,
                                   options_,
                                   duration_,
                                   closure_,
                                   TsGroupedWindowAggOp::Max,
                                   value_feature,
                                   "DataFrameTsWindowAccessor::max");
}

DataFrame DataFrameTsWindowAccessor::count(std::string_view value_feature) const {
  return grouped_window_aggregate_(frame_,
                                   features_,
                                   options_,
                                   duration_,
                                   closure_,
                                   TsGroupedWindowAggOp::Count,
                                   value_feature,
                                   "DataFrameTsWindowAccessor::count");
}

DataFrameTsResampler DataFrameTsAccessor::resample(std::string_view time_feature,
                                                   std::string_view freq,
                                                   std::int64_t step) const {
  return resample(time_feature, pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step});
}

DataFrameTsResampler DataFrameTsAccessor::resample(
    std::string_view time_feature,
    const pluss::TimeIndex::ResampleOptions& options) const {
  return DataFrameTsResampler(frame_, features_, owned_name(time_feature), options, options_);
}

DataFrameTsResampler DataFrameTsAccessor::resample(std::string_view freq,
                                                   std::int64_t step) const {
  return resample(pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step});
}

DataFrameTsResampler DataFrameTsAccessor::resample(
    const pluss::TimeIndex::ResampleOptions& options) const {
  return DataFrameTsResampler(frame_, features_, std::string{}, options, options_);
}

DataFrame DataFrameTsAccessor::resample(std::string_view time_feature,
                                        std::string_view freq,
                                        pluss::subsp::stats::ts::ResampleOp op,
                                        std::int64_t step) const {
  return resample(time_feature, pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step}, op);
}

DataFrame DataFrameTsAccessor::resample(
    std::string_view time_feature,
    const pluss::TimeIndex::ResampleOptions& options,
    pluss::subsp::stats::ts::ResampleOp op) const {
  if (time_feature.empty()) {
    return resample(options, op);
  }
  PLUSS_CHECK(!features_.empty(), "DataFrameTsAccessor::resample: expected at least one feature");
  const auto axis_feature = owned_name(time_feature);
  (void)require_time_axis_index_(frame_, axis_feature, "DataFrameTsAccessor::resample");
  const auto axis = frame_.features(axis_feature);
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  Thread out_cases;
  for (const auto& name : features_) {
    const auto sampled = frame_.features(name).ts().resample(axis, options, op);
    if (!out_cases.defined()) {
      out_cases = sampled.labels;
    }
    out.emplace_back(name, sampled.values);
  }
  return make_frame_from_columns_(std::move(out), out_cases);
}

DataFrame DataFrameTsAccessor::resample(std::string_view freq,
                                        pluss::subsp::stats::ts::ResampleOp op,
                                        std::int64_t step) const {
  return resample(pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step}, op);
}

DataFrame DataFrameTsAccessor::resample(
    const pluss::TimeIndex::ResampleOptions& options,
    pluss::subsp::stats::ts::ResampleOp op) const {
  PLUSS_CHECK(!features_.empty(), "DataFrameTsAccessor::resample: expected at least one feature");
  const auto axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::resample");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  Thread out_cases;
  for (const auto& name : features_) {
    const auto sampled = frame_.features(name).ts().resample(axis, options, op);
    if (!out_cases.defined()) {
      out_cases = sampled.labels;
    }
    out.emplace_back(name, sampled.values);
  }
  return make_frame_from_columns_(std::move(out), out_cases);
}

DataFrame DataFrameTsAccessor::asfreq(std::string_view time_feature,
                                      std::string_view freq,
                                      std::int64_t step,
                                      pluss::subsp::stats::ts::GapFillMode fill_mode,
                                      std::optional<double> constant,
                                      pluss::subsp::stats::ts::ResampleOp op) const {
  return asfreq(time_feature, pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step}, fill_mode, constant, op);
}

DataFrame DataFrameTsAccessor::asfreq(
    std::string_view time_feature,
    const pluss::TimeIndex::ResampleOptions& options,
    pluss::subsp::stats::ts::GapFillMode fill_mode,
    std::optional<double> constant,
    pluss::subsp::stats::ts::ResampleOp op) const {
  if (time_feature.empty()) {
    return asfreq(options, fill_mode, constant, op);
  }
  auto out = resample(time_feature, options, op);
  for (const auto& name : features_) {
    out.set_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), fill_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::asfreq(std::string_view freq,
                                      std::int64_t step,
                                      pluss::subsp::stats::ts::GapFillMode fill_mode,
                                      std::optional<double> constant,
                                      pluss::subsp::stats::ts::ResampleOp op) const {
  return asfreq(pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step}, fill_mode, constant, op);
}

DataFrame DataFrameTsAccessor::asfreq(
    const pluss::TimeIndex::ResampleOptions& options,
    pluss::subsp::stats::ts::GapFillMode fill_mode,
    std::optional<double> constant,
    pluss::subsp::stats::ts::ResampleOp op) const {
  auto out = resample(options, op);
  for (const auto& name : features_) {
    out.set_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), fill_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::insert_gaps(std::string_view time_feature, std::string_view freq, std::int64_t step) const {
  if (time_feature.empty()) {
    return insert_gaps(freq, step);
  }
  const auto source_ti = require_time_axis_index_(frame_, time_feature, "DataFrameTsAccessor::insert_gaps");
  const auto bins = source_ti.resample_bins(freq, step);
  return align_to(time_feature, Thread(bins.labels), TsAlignMode::Exact);
}

DataFrame DataFrameTsAccessor::insert_gaps(std::string_view freq, std::int64_t step) const {
  const auto source_axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::insert_gaps");
  const auto source_ti = thread_to_timeindex_(source_axis, "DataFrameTsAccessor::insert_gaps");
  const auto bins = source_ti.resample_bins(freq, step);
  return align_to(Thread(bins.labels), TsAlignMode::Exact);
}

DataFrame DataFrameTsAccessor::align_to(std::string_view time_feature, const Thread& target_axis, TsAlignMode mode) const {
  if (time_feature.empty()) {
    return align_to(target_axis, mode);
  }
  const auto axis = require_time_axis_thread_(target_axis, "DataFrameTsAccessor::align_to");
  (void)require_time_axis_index_(frame_, time_feature, "DataFrameTsAccessor::align_to");
  const auto source_axis = frame_.features(time_feature);

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().align_to(source_axis, axis, mode));
  }
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrame DataFrameTsAccessor::align_to(const Thread& target_axis, TsAlignMode mode) const {
  const auto axis = require_time_axis_thread_(target_axis, "DataFrameTsAccessor::align_to");
  const auto source_axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::align_to");

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().align_to(source_axis, axis, mode));
  }
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrameTsResampler::DataFrameTsResampler(DataFrame frame,
                                           std::vector<std::string> features,
                                           std::string time_feature,
                                           std::string freq,
                                           std::int64_t step,
                                           TsOptions options)
    : DataFrameTsResampler(std::move(frame),
                           std::move(features),
                           std::move(time_feature),
                           pluss::TimeIndex::ResampleOptions{.freq = std::move(freq), .step = step},
                           std::move(options)) {}

DataFrameTsResampler::DataFrameTsResampler(
    DataFrame frame,
    std::vector<std::string> features,
    std::string time_feature,
    pluss::TimeIndex::ResampleOptions options,
    TsOptions ts_options)
    : frame_(std::move(frame)),
      features_(std::move(features)),
      time_feature_(std::move(time_feature)),
      freq_(options.freq),
      step_(options.step),
      options_(std::move(options)),
      ts_options_(std::move(ts_options)) {
  PLUSS_CHECK(!features_.empty(), "DataFrameTsResampler: expected at least one feature");
  PLUSS_CHECK(!options_.freq.empty(), "DataFrameTsResampler: freq cannot be empty");
  PLUSS_CHECK(options_.step > 0, "DataFrameTsResampler: step must be positive");
  validate_ts_options_(frame_, ts_options_, "DataFrameTsResampler");
}

DataFrame DataFrameTsResampler::aggregate_(pluss::subsp::stats::ts::ResampleOp op) const {
  return time_feature_.empty()
             ? DataFrameTsAccessor(frame_, features_, ts_options_).resample(options_, op)
             : DataFrameTsAccessor(frame_, features_, ts_options_).resample(time_feature_, options_, op);
}

DataFrame DataFrameTsResampler::mean() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Mean);
}

DataFrame DataFrameTsResampler::sum() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Sum);
}

DataFrame DataFrameTsResampler::min() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Min);
}

DataFrame DataFrameTsResampler::max() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Max);
}

DataFrame DataFrameTsResampler::first() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::First);
}

DataFrame DataFrameTsResampler::last() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Last);
}

DataFrame DataFrameTsResampler::count() const {
  return aggregate_(pluss::subsp::stats::ts::ResampleOp::Count);
}

DataFrame DataFrameTsResampler::asfreq(std::optional<double> fill_value) const {
  const auto source_axis = time_feature_.empty()
                               ? ts_source_axis_(frame_, ts_options_, "DataFrameTsResampler::asfreq")
                               : frame_.features(time_feature_);
  const auto target_axis = time_feature_.empty()
                               ? resample_target_axis_(source_axis, options_, "DataFrameTsResampler::asfreq")
                               : resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::asfreq");
  auto out = time_feature_.empty()
                 ? DataFrameTsAccessor(frame_, features_, ts_options_).align_to(target_axis, TsAlignMode::Exact)
                 : DataFrameTsAccessor(frame_, features_, ts_options_).align_to(time_feature_, target_axis, TsAlignMode::Exact);
  if (!fill_value.has_value()) {
    return out;
  }
  const auto source_ti = thread_to_timeindex_(source_axis, "DataFrameTsResampler::asfreq");
  const auto target_ti = thread_to_timeindex_(target_axis, "DataFrameTsResampler::asfreq");
  const auto n_source = source_ti.size();
  const auto n_target = target_ti.size();
  Series idx = pluss::full({n_target}, -1.0, ScalarType::Int64);
  if (n_source > 0 && n_target > 0) {
    idx = target_ti.join_asof(source_ti, pluss::TimeIndex::AsofDirection::Backward, nullptr, true);
  }
  auto exact = pluss::Series::zeros({n_target}, pluss::codem::ScalarType::Bool);
  if (n_source > 0 && n_target > 0) {
    const Series valid = idx.ge(0.0);
    const Series source_ticks = source_ti.timeline().epoch_nanos().reshape({n_source});
    const Series target_ticks = target_ti.timeline().epoch_nanos().reshape({n_target});
    const Series safe_idx = idx.to(ScalarType::Float64).clamp(0.0, static_cast<double>(n_source - 1)).to(ScalarType::Int64);
    const Series gathered_ticks = source_ticks.index_select(0, safe_idx);
    exact = valid.logical_and(gathered_ticks.eq(target_ticks));
  }
  for (const auto& name : features_) {
    out.set_(name, Thread(out.features(name).as_series().where(exact.logical_not(), *fill_value)));
  }
  return out;
}

DataFrame DataFrameTsResampler::ffill() const {
  const auto target_axis = time_feature_.empty()
                               ? resample_target_axis_(ts_source_axis_(frame_, ts_options_, "DataFrameTsResampler::ffill"),
                                                       options_,
                                                       "DataFrameTsResampler::ffill")
                               : resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::ffill");
  return time_feature_.empty()
             ? DataFrameTsAccessor(frame_, features_, ts_options_).align_to(target_axis, TsAlignMode::Backward)
             : DataFrameTsAccessor(frame_, features_, ts_options_).align_to(time_feature_, target_axis, TsAlignMode::Backward);
}

DataFrame DataFrameTsResampler::bfill() const {
  const auto target_axis = time_feature_.empty()
                               ? resample_target_axis_(ts_source_axis_(frame_, ts_options_, "DataFrameTsResampler::bfill"),
                                                       options_,
                                                       "DataFrameTsResampler::bfill")
                               : resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::bfill");
  return time_feature_.empty()
             ? DataFrameTsAccessor(frame_, features_, ts_options_).align_to(target_axis, TsAlignMode::Forward)
             : DataFrameTsAccessor(frame_, features_, ts_options_).align_to(time_feature_, target_axis, TsAlignMode::Forward);
}

DataFrame DataFrameTsResampler::nearest() const {
  const auto target_axis = time_feature_.empty()
                               ? resample_target_axis_(ts_source_axis_(frame_, ts_options_, "DataFrameTsResampler::nearest"),
                                                       options_,
                                                       "DataFrameTsResampler::nearest")
                               : resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::nearest");
  return time_feature_.empty()
             ? DataFrameTsAccessor(frame_, features_, ts_options_).align_to(target_axis, TsAlignMode::Nearest)
             : DataFrameTsAccessor(frame_, features_, ts_options_).align_to(time_feature_, target_axis, TsAlignMode::Nearest);
}

DataFrame DataFrameTsAccessor::align_to(std::string_view time_feature,
                                        const Thread& target_axis,
                                        TsAlignMode mode,
                                        TsReindexFill fill,
                                        std::optional<double> constant) const {
  if (time_feature.empty()) {
    return align_to(target_axis, mode, fill, constant);
  }
  auto out = align_to(time_feature, target_axis, mode);
  if (fill == TsReindexFill::None) {
    return out;
  }
  pluss::subsp::stats::ts::GapFillMode gap_mode = pluss::subsp::stats::ts::GapFillMode::Forward;
  switch (fill) {
    case TsReindexFill::Forward:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Forward;
      break;
    case TsReindexFill::Backward:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Backward;
      break;
    case TsReindexFill::Zero:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Zero;
      break;
    case TsReindexFill::Constant:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Constant;
      PLUSS_CHECK(constant.has_value(), "DataFrameTsAccessor::align_to: constant fill requires a value");
      break;
    case TsReindexFill::None:
    default:
      break;
  }
  for (const auto& name : features_) {
    out.set_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), gap_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::align_to(const Thread& target_axis,
                                        TsAlignMode mode,
                                        TsReindexFill fill,
                                        std::optional<double> constant) const {
  auto out = align_to(target_axis, mode);
  if (fill == TsReindexFill::None) {
    return out;
  }
  pluss::subsp::stats::ts::GapFillMode gap_mode = pluss::subsp::stats::ts::GapFillMode::Forward;
  switch (fill) {
    case TsReindexFill::Forward:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Forward;
      break;
    case TsReindexFill::Backward:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Backward;
      break;
    case TsReindexFill::Zero:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Zero;
      break;
    case TsReindexFill::Constant:
      gap_mode = pluss::subsp::stats::ts::GapFillMode::Constant;
      PLUSS_CHECK(constant.has_value(), "DataFrameTsAccessor::align_to: constant fill requires a value");
      break;
    case TsReindexFill::None:
    default:
      break;
  }
  for (const auto& name : features_) {
    out.set_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), gap_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::add(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, options_.axis, mode, 0);
}

DataFrame DataFrameTsAccessor::sub(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, options_.axis, mode, 1);
}

DataFrame DataFrameTsAccessor::mul(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, options_.axis, mode, 2);
}

DataFrame DataFrameTsAccessor::div(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, options_.axis, mode, 3);
}

DataFrame DataFrameTsAccessor::calendar(std::string_view time_feature) const {
  PLUSS_CHECK(frame_.get(time_feature).has_value(), "DataFrameTsAccessor::calendar: unknown time feature");
  const auto axis = frame_.features(time_feature);
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              "DataFrameTsAccessor::calendar: time feature must be Timeline/TimeIndex");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(5);
  out.emplace_back(feature_suffix_(time_feature, "year"), axis.year());
  out.emplace_back(feature_suffix_(time_feature, "month"), axis.month());
  out.emplace_back(feature_suffix_(time_feature, "day"), axis.day());
  out.emplace_back(feature_suffix_(time_feature, "hour"), axis.hour());
  out.emplace_back(feature_suffix_(time_feature, "weekday"), axis.to_note_weekday());
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::calendar() const {
  const auto axis = ts_source_axis_(frame_, options_, "DataFrameTsAccessor::calendar");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(5);
  out.emplace_back("axis_year", axis.year());
  out.emplace_back("axis_month", axis.month());
  out.emplace_back("axis_day", axis.day());
  out.emplace_back("axis_hour", axis.hour());
  out.emplace_back("axis_weekday", axis.to_note_weekday());
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrame DataFrameTsAccessor::lag_block(pluss::msharp::RangeView::Int lags) const {
  PLUSS_CHECK(lags.size() > 0, "DataFrameTsAccessor::lag_block: expected at least one lag");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size() * lags.size());
  for (const auto& name : features_) {
    const auto block = frame_.features(name).ts().lag_block(lags);
    for (const auto& block_name : block.features()) {
      out.emplace_back(std::string(name) + "_" + block_name, block.features(block_name));
    }
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}

DataFrame DataFrameTsAccessor::rolling_block(pluss::msharp::RangeView::Int windows,
                                             const TsRollingBlockOptions& options) const {
  PLUSS_CHECK(windows.size() > 0, "DataFrameTsAccessor::rolling_block: expected at least one window");
  std::vector<std::pair<std::string, Thread>> out;
  for (const auto& name : features_) {
    const auto block = frame_.features(name).ts().rolling_block(windows, options);
    for (const auto& block_name : block.features()) {
      out.emplace_back(std::string(name) + "_" + block_name, block.features(block_name));
    }
  }
  return make_frame_from_columns_(std::move(out), frame_.label().values());
}
