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
      PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
                  std::string(where) + ": no features supplied; current frame contains non-numeric Series feature '" +
                      name + "'. Select time-series value feature(s) first, e.g. df.features({\"price\"}).ts()");
      out.push_back(name);
    }
    return out;
  }

  std::unordered_set<std::string> seen;
  std::vector<std::string> out;
  out.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(frame.has_feature(name), std::string(where) + ": unknown feature '" + name + "'");
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
  PLUSS_CHECK(frame.has_feature(time_feature), std::string(where) + ": unknown time feature '" + std::string(time_feature) + "'");
  return time_axis_to_index_(frame.features(time_feature), where);
}

pluss::TimeIndex require_case_time_axis_index_(const DataFrame& frame, std::string_view where) {
  const auto axis = frame.caselbl();
  PLUSS_CHECK(axis.defined(), std::string(where) + ": cases are undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": cases must be Timeline/TimeIndex. Use df.set_cases(\"date\") first");
  return axis.kind() == ThreadKind::TimeIndex ? axis.as_timeindex() : axis.as_timeline().to_timeindex(false);
}

Thread require_time_axis_thread_(const Thread& axis, std::string_view where) {
  PLUSS_CHECK(axis.defined(), std::string(where) + ": target axis is undefined");
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": target axis must be Timeline or TimeIndex");
  return axis;
}

pluss::TimeIndex thread_to_timeindex_(const Thread& axis, std::string_view where) {
  PLUSS_CHECK(axis.kind() == ThreadKind::Timeline || axis.kind() == ThreadKind::TimeIndex,
              std::string(where) + ": axis must be Timeline or TimeIndex");
  return axis.kind() == ThreadKind::TimeIndex ? axis.as_timeindex() : axis.as_timeline().to_timeindex(false);
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
                           TsAlignMode mode,
                           int op) {
  const auto left_axis = left.caselbl();
  const auto right_axis = right.caselbl();
  (void)time_axis_to_index_(left_axis, "DataFrameTsAccessor::binary");
  (void)time_axis_to_index_(right_axis, "DataFrameTsAccessor::binary");

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features.size());
  for (const auto& name : features) {
    PLUSS_CHECK(right.has_feature(name), "DataFrameTsAccessor::binary: right frame is missing feature '" + name + "'");
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

} // namespace

DataFrameTsAccessor::DataFrameTsAccessor(DataFrame frame, std::vector<std::string> features)
    : frame_(std::move(frame)), features_(std::move(features)) {
  PLUSS_CHECK(!features_.empty(), "DataFrameTsAccessor: expected at least one feature");
}

DataFrameTsAccessor DataFrame::ts(std::string_view feature) const {
  const auto single = std::array<std::string_view, 1>{feature};
  return ts(pluss::msharp::RangeView::Of<std::string_view>(
      std::span<const std::string_view>(single.data(), single.size())));
}

DataFrameTsAccessor DataFrame::ts(std::initializer_list<std::string_view> features) const {
  return ts(pluss::msharp::RangeView::Of<std::string_view>(features));
}

DataFrameTsAccessor DataFrame::ts(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return DataFrameTsAccessor(*this, normalize_ts_features_(*this, features, "DataFrame::ts"));
}

DataFrame DataFrameTsAccessor::returns(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().returns(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::log_returns(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().log_returns(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::diff(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().diff(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::lag(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lag(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::lead(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lead(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::rolling(
    std::string_view duration,
    WindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  const auto axis = frame_.caselbl();
  (void)require_case_time_axis_index_(frame_, "DataFrameTsAccessor::rolling");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().rolling(axis, duration, statistic, options, closure));
  }
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrame DataFrameTsAccessor::grouped(std::string_view group_feature,
                                       std::int64_t window,
                                       WindowStatistic statistic,
                                       const WindowOptions& options) const {
  PLUSS_CHECK(frame_.has_feature(group_feature),
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::expanding(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().expanding(statistic, options));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().zscore_rolling(window, min_periods, epsilon));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().fill_gaps(mode, constant));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}

DataFrame DataFrameTsAccessor::sort(bool ascending, bool nulls_last) const {
  (void)require_case_time_axis_index_(frame_, "DataFrameTsAccessor::sort");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name));
  }
  return make_frame_from_columns_(std::move(out), frame_.caselbl()).sort_cases(ascending, nulls_last);
}

DataFrameTsResampler DataFrameTsAccessor::resample(std::string_view time_feature,
                                                   std::string_view freq,
                                                   std::int64_t step) const {
  return resample(time_feature, pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step});
}

DataFrameTsResampler DataFrameTsAccessor::resample(
    std::string_view time_feature,
    const pluss::TimeIndex::ResampleOptions& options) const {
  return DataFrameTsResampler(frame_, features_, owned_name(time_feature), options);
}

DataFrameTsResampler DataFrameTsAccessor::resample(std::string_view freq,
                                                   std::int64_t step) const {
  return resample(pluss::TimeIndex::ResampleOptions{.freq = std::string(freq), .step = step});
}

DataFrameTsResampler DataFrameTsAccessor::resample(
    const pluss::TimeIndex::ResampleOptions& options) const {
  return DataFrameTsResampler(frame_, features_, std::string{}, options);
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
  const auto axis = frame_.caselbl();
  (void)require_case_time_axis_index_(frame_, "DataFrameTsAccessor::resample");
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
    out.set_features_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), fill_mode, constant)));
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
    out.set_features_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), fill_mode, constant)));
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
  const auto source_ti = require_case_time_axis_index_(frame_, "DataFrameTsAccessor::insert_gaps");
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
  (void)require_case_time_axis_index_(frame_, "DataFrameTsAccessor::align_to");
  const auto source_axis = frame_.caselbl();

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
                                           std::int64_t step)
    : DataFrameTsResampler(std::move(frame),
                           std::move(features),
                           std::move(time_feature),
                           pluss::TimeIndex::ResampleOptions{.freq = std::move(freq), .step = step}) {}

DataFrameTsResampler::DataFrameTsResampler(
    DataFrame frame,
    std::vector<std::string> features,
    std::string time_feature,
    pluss::TimeIndex::ResampleOptions options)
    : frame_(std::move(frame)),
      features_(std::move(features)),
      time_feature_(std::move(time_feature)),
      freq_(options.freq),
      step_(options.step),
      options_(std::move(options)) {
  PLUSS_CHECK(!features_.empty(), "DataFrameTsResampler: expected at least one feature");
  PLUSS_CHECK(!options_.freq.empty(), "DataFrameTsResampler: freq cannot be empty");
  PLUSS_CHECK(options_.step > 0, "DataFrameTsResampler: step must be positive");
}

DataFrame DataFrameTsResampler::aggregate_(pluss::subsp::stats::ts::ResampleOp op) const {
  return time_feature_.empty()
             ? DataFrameTsAccessor(frame_, features_).resample(options_, op)
             : DataFrameTsAccessor(frame_, features_).resample(time_feature_, options_, op);
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
  const auto target_axis = resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::asfreq");
  auto out = DataFrameTsAccessor(frame_, features_).align_to(time_feature_, target_axis, TsAlignMode::Exact);
  if (!fill_value.has_value()) {
    return out;
  }
  const auto source_ti = time_feature_.empty()
                             ? require_case_time_axis_index_(frame_, "DataFrameTsResampler::asfreq")
                             : require_time_axis_index_(frame_, time_feature_, "DataFrameTsResampler::asfreq");
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
    out.set_features_(name, Thread(out.features(name).as_series().where(exact.logical_not(), *fill_value)));
  }
  return out;
}

DataFrame DataFrameTsResampler::ffill() const {
  const auto target_axis = resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::ffill");
  return DataFrameTsAccessor(frame_, features_).align_to(time_feature_, target_axis, TsAlignMode::Backward);
}

DataFrame DataFrameTsResampler::bfill() const {
  const auto target_axis = resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::bfill");
  return DataFrameTsAccessor(frame_, features_).align_to(time_feature_, target_axis, TsAlignMode::Forward);
}

DataFrame DataFrameTsResampler::nearest() const {
  const auto target_axis = resample_target_axis_(frame_, time_feature_, options_, "DataFrameTsResampler::nearest");
  return DataFrameTsAccessor(frame_, features_).align_to(time_feature_, target_axis, TsAlignMode::Nearest);
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
    out.set_features_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), gap_mode, constant)));
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
    out.set_features_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), gap_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::add(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, mode, 0);
}

DataFrame DataFrameTsAccessor::sub(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, mode, 1);
}

DataFrame DataFrameTsAccessor::mul(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, mode, 2);
}

DataFrame DataFrameTsAccessor::div(const DataFrame& other, TsAlignMode mode) const {
  return ts_binary_frame_(frame_, other, features_, mode, 3);
}

DataFrame DataFrameTsAccessor::calendar(std::string_view time_feature) const {
  PLUSS_CHECK(frame_.has_feature(time_feature), "DataFrameTsAccessor::calendar: unknown time feature");
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
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
  return make_frame_from_columns_(std::move(out), frame_.caselbl());
}
