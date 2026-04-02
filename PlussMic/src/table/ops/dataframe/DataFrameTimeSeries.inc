namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

std::vector<std::string> normalize_ts_features_(const DataFrame& frame,
                                                pluss::msharp::RangeView::Of<std::string_view> features,
                                                std::string_view where) {
  PLUSS_CHECK(features.size() > 0, std::string(where) + ": at least one feature is required");
  std::unordered_set<std::string> seen;
  std::vector<std::string> out;
  out.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(frame.has_feature(name), std::string(where) + ": unknown feature '" + name + "'");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    out.push_back(name);
  }
  return out;
}

pluss::TimeIndex require_time_axis_index_(const DataFrame& frame, std::string_view time_feature, std::string_view where) {
  PLUSS_CHECK(frame.has_feature(time_feature), std::string(where) + ": unknown time feature '" + std::string(time_feature) + "'");
  return time_axis_to_index_(frame.features(time_feature), where);
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

pluss::TimeIndex::AsofDirection to_asof_direction_(TsAlignMode mode) {
  switch (mode) {
    case TsAlignMode::Backward:
      return pluss::TimeIndex::AsofDirection::Backward;
    case TsAlignMode::Forward:
      return pluss::TimeIndex::AsofDirection::Forward;
    case TsAlignMode::Nearest:
      return pluss::TimeIndex::AsofDirection::Nearest;
    case TsAlignMode::Exact:
    default:
      return pluss::TimeIndex::AsofDirection::Backward;
  }
}

Series gather_with_missing_(const Series& source, const Series& index, const Series& valid_mask) {
  const auto n = source.numel();
  const auto out_n = index.numel();
  const auto out_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(source.platform());
  if (n == 0 || out_n == 0) {
    return pluss::full({out_n}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  }
  const Series safe_idx = index.to(ScalarType::Float64).clamp(0.0, static_cast<double>(n - 1)).to(ScalarType::Int64);
  const Series gathered = source.to(ScalarType::Float64).index_select(0, safe_idx);
  return gathered.masked_fill(valid_mask.logical_not(), std::numeric_limits<double>::quiet_NaN());
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
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::log_returns(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().log_returns(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::diff(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().diff(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::lag(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lag(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::lead(std::int64_t periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, frame_.features(name).ts().lead(periods));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_mean(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_mean(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_std(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_std(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_sum(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_sum(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_var(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_var(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_min(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_min(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_max(std::int64_t window, std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::rolling_max(frame_.features(name).as_series(), window, min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::ema(double alpha) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::ema(frame_.features(name).as_series(), alpha)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::ewm_mean(double alpha) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::ewm_mean(frame_.features(name).as_series(), alpha)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::ewm_std(double alpha, double epsilon) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::ewm_std(frame_.features(name).as_series(), alpha, epsilon)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::expanding_mean(std::int64_t min_periods) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::expanding_mean(frame_.features(name).as_series(), min_periods)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::expanding_std(std::int64_t min_periods, double epsilon) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::expanding_std(frame_.features(name).as_series(), min_periods, epsilon)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods, double epsilon) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(
        name,
        Thread(pluss::subsp::stats::ts::zscore_rolling(frame_.features(name).as_series(), window, min_periods, epsilon)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode, std::optional<double> constant) const {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    out.emplace_back(name, Thread(pluss::subsp::stats::ts::fill_gaps(frame_.features(name).as_series(), mode, constant)));
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::resample(std::string_view time_feature,
                                        std::string_view freq,
                                        pluss::subsp::stats::ts::ResampleOp op,
                                        std::int64_t step) const {
  const auto ti = require_time_axis_index_(frame_, time_feature, "DataFrameTsAccessor::resample");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  std::optional<pluss::Timeline> labels{};
  for (const auto& name : features_) {
    const auto result = pluss::subsp::stats::ts::resample(ti, frame_.features(name).as_series(), freq, op, step);
    if (!labels.has_value() && result.labels.has_value()) {
      labels = result.labels;
    }
    out.emplace_back(name, Thread(result.values));
  }
  PLUSS_CHECK(labels.has_value(), "DataFrameTsAccessor::resample: expected non-empty labels");
  return make_frame_from_columns_(std::move(out), Thread(*labels));
}

DataFrame DataFrameTsAccessor::asfreq(std::string_view time_feature,
                                      std::string_view freq,
                                      std::int64_t step,
                                      pluss::subsp::stats::ts::GapFillMode fill_mode,
                                      std::optional<double> constant,
                                      pluss::subsp::stats::ts::ResampleOp op) const {
  auto out = resample(time_feature, freq, op, step);
  for (const auto& name : features_) {
    out.set_features_(name, Thread(pluss::subsp::stats::ts::fill_gaps(out.features(name).as_series(), fill_mode, constant)));
  }
  return out;
}

DataFrame DataFrameTsAccessor::insert_gaps(std::string_view time_feature, std::string_view freq, std::int64_t step) const {
  const auto source_ti = require_time_axis_index_(frame_, time_feature, "DataFrameTsAccessor::insert_gaps");
  const auto bins = source_ti.resample_bins(freq, step);
  return align_to(time_feature, Thread(bins.labels), TsAlignMode::Exact);
}

DataFrame DataFrameTsAccessor::align_to(std::string_view time_feature, const Thread& target_axis, TsAlignMode mode) const {
  const auto source_ti = require_time_axis_index_(frame_, time_feature, "DataFrameTsAccessor::align_to");
  const auto axis = require_time_axis_thread_(target_axis, "DataFrameTsAccessor::align_to");
  const auto target_ti = thread_to_timeindex_(axis, "DataFrameTsAccessor::align_to");
  const std::int64_t n_source = source_ti.size();
  const std::int64_t n_target = target_ti.size();

  Series idx;
  if (n_source == 0) {
    idx = pluss::full({n_target}, -1.0, ScalarType::Int64);
  } else {
    idx = target_ti.join_asof(source_ti, to_asof_direction_(mode), nullptr, true);
  }
  Series valid = idx.ge(0.0);
  if (mode == TsAlignMode::Exact && n_source > 0) {
    const Series source_ticks = source_ti.timeline().epoch_nanos().reshape({n_source});
    const Series target_ticks = target_ti.timeline().epoch_nanos().reshape({n_target});
    const Series safe_idx = idx.to(ScalarType::Float64).clamp(0.0, static_cast<double>(n_source - 1)).to(ScalarType::Int64);
    const Series gathered_ticks = source_ticks.index_select(0, safe_idx);
    valid = valid.logical_and(gathered_ticks.eq(target_ticks));
  }

  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size());
  for (const auto& name : features_) {
    const Series aligned = gather_with_missing_(frame_.features(name).as_series(), idx, valid);
    out.emplace_back(name, Thread(aligned));
  }
  return make_frame_from_columns_(std::move(out), axis);
}

DataFrame DataFrameTsAccessor::reindex_time(std::string_view time_feature,
                                            const Thread& target_axis,
                                            TsAlignMode mode,
                                            TsReindexFill fill,
                                            std::optional<double> constant) const {
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
      PLUSS_CHECK(constant.has_value(), "DataFrameTsAccessor::reindex_time: constant fill requires a value");
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
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::lag_block(pluss::msharp::RangeView::Int lags) const {
  PLUSS_CHECK(lags.size() > 0, "DataFrameTsAccessor::lag_block: expected at least one lag");
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(features_.size() * lags.size());
  for (const auto& name : features_) {
    const auto series = frame_.features(name).as_series();
    for (const auto lag : lags) {
      out.emplace_back(feature_suffix_(name, "lag", lag), Thread(pluss::subsp::stats::ts::lag(series, lag)));
    }
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}

DataFrame DataFrameTsAccessor::rolling_block(pluss::msharp::RangeView::Int windows,
                                             const TsRollingBlockOptions& options) const {
  PLUSS_CHECK(windows.size() > 0, "DataFrameTsAccessor::rolling_block: expected at least one window");
  std::vector<std::pair<std::string, Thread>> out;
  for (const auto& name : features_) {
    const auto series = frame_.features(name).as_series();
    for (const auto w : windows) {
      if (options.include_mean) {
        out.emplace_back(feature_suffix_(name, "roll_mean_w", w), Thread(pluss::subsp::stats::ts::rolling_mean(series, w, options.min_periods)));
      }
      if (options.include_std) {
        out.emplace_back(feature_suffix_(name, "roll_std_w", w), Thread(pluss::subsp::stats::ts::rolling_std(series, w, options.min_periods)));
      }
      if (options.include_min) {
        out.emplace_back(feature_suffix_(name, "roll_min_w", w), Thread(pluss::subsp::stats::ts::rolling_min(series, w, options.min_periods)));
      }
      if (options.include_max) {
        out.emplace_back(feature_suffix_(name, "roll_max_w", w), Thread(pluss::subsp::stats::ts::rolling_max(series, w, options.min_periods)));
      }
      if (options.include_sum) {
        out.emplace_back(feature_suffix_(name, "roll_sum_w", w), Thread(pluss::subsp::stats::ts::rolling_sum(series, w, options.min_periods)));
      }
      if (options.include_var) {
        out.emplace_back(feature_suffix_(name, "roll_var_w", w), Thread(pluss::subsp::stats::ts::rolling_var(series, w, options.min_periods)));
      }
    }
  }
  return make_frame_from_columns_(std::move(out), frame_.cases());
}
