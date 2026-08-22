#pragma once

#include "pluss/ai/lymph/model.h"

#include <cmath>

namespace pluss::ai::lymph::shape {

struct ProfileOptions {
  double skew_threshold = 1.0;
  double heavy_tail_threshold = 3.0;
  double low_variance_threshold = 1e-12;
};

class Plan final {
 public:
  Plan(std::vector<std::string> feature_names, prep::QuantileMapOptions options = {})
      : feature_names_(std::move(feature_names)), options_(options) {
    options_.output = prep::QuantileMapOutput::Gaussian;
  }

  [[nodiscard]] bool empty() const noexcept { return feature_names_.empty(); }
  [[nodiscard]] std::int64_t feature_count() const noexcept {
    return static_cast<std::int64_t>(feature_names_.size());
  }
  [[nodiscard]] const std::vector<std::string>& features() const noexcept { return feature_names_; }
  [[nodiscard]] const prep::QuantileMapOptions& options() const noexcept { return options_; }

  Model& apply(Model& model) const {
    if (feature_names_.empty()) return model;
    auto prep = model.prepare();
    prep.gaussianize(prep.names(feature_names_), options_);
    return model;
  }

 private:
  std::vector<std::string> feature_names_{};
  prep::QuantileMapOptions options_{};
};

class Result final {
 public:
  Result(std::vector<std::string> feature_names,
         std::int64_t sample_count,
         Series mean,
         Series stddev,
         Series minimum,
         Series maximum,
         Series skewness,
         Series excess_kurtosis,
         Series missing_rate,
         Series negative_rate,
         Series zero_rate,
         Series gaussianize_candidate)
      : feature_names_(std::move(feature_names)),
        sample_count_(sample_count),
        mean_(std::move(mean)),
        stddev_(std::move(stddev)),
        minimum_(std::move(minimum)),
        maximum_(std::move(maximum)),
        skewness_(std::move(skewness)),
        excess_kurtosis_(std::move(excess_kurtosis)),
        missing_rate_(std::move(missing_rate)),
        negative_rate_(std::move(negative_rate)),
        zero_rate_(std::move(zero_rate)),
        gaussianize_candidate_(std::move(gaussianize_candidate)) {}

  [[nodiscard]] const std::vector<std::string>& features() const noexcept { return feature_names_; }
  [[nodiscard]] std::int64_t sample_count() const noexcept { return sample_count_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept {
    return static_cast<std::int64_t>(feature_names_.size());
  }
  [[nodiscard]] const Series& mean() const noexcept { return mean_; }
  [[nodiscard]] const Series& stddev() const noexcept { return stddev_; }
  [[nodiscard]] const Series& minimum() const noexcept { return minimum_; }
  [[nodiscard]] const Series& maximum() const noexcept { return maximum_; }
  [[nodiscard]] const Series& skewness() const noexcept { return skewness_; }
  [[nodiscard]] const Series& excess_kurtosis() const noexcept { return excess_kurtosis_; }
  [[nodiscard]] const Series& missing_rate() const noexcept { return missing_rate_; }
  [[nodiscard]] const Series& negative_rate() const noexcept { return negative_rate_; }
  [[nodiscard]] const Series& zero_rate() const noexcept { return zero_rate_; }
  [[nodiscard]] const Series& gaussianize_candidate() const noexcept { return gaussianize_candidate_; }

  [[nodiscard]] pluss::table::DataFrame board() const {
    const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(mean_.platform());
    return pluss::table::DataFrame({
        {"feature", pluss::table::Thread(pluss::make_note(feature_names_))},
        {"sample_count", pluss::table::Thread(pluss::series::full({feature_count()}, static_cast<double>(sample_count_), i64_cfg))},
        {"mean", pluss::table::Thread(mean_)},
        {"stddev", pluss::table::Thread(stddev_)},
        {"min", pluss::table::Thread(minimum_)},
        {"max", pluss::table::Thread(maximum_)},
        {"skewness", pluss::table::Thread(skewness_)},
        {"excess_kurtosis", pluss::table::Thread(excess_kurtosis_)},
        {"missing_rate", pluss::table::Thread(missing_rate_)},
        {"negative_rate", pluss::table::Thread(negative_rate_)},
        {"zero_rate", pluss::table::Thread(zero_rate_)},
        {"gaussianize_candidate", pluss::table::Thread(gaussianize_candidate_)},
    });
  }

  [[nodiscard]] Plan plan(ProfileOptions options = {}) const {
    const Series variable = stddev_.gt(options.low_variance_threshold);
    const Series skewed = skewness_.abs().gt(options.skew_threshold);
    const Series tailed = excess_kurtosis_.abs().gt(options.heavy_tail_threshold);
    const Series candidate = variable.logical_and(skewed.logical_or(tailed));
    const std::vector<double> mask = candidate.to(pluss::pCPU, ScalarType::Float64)
                                         .reshape({candidate.numel()})
                                         .to_vec<double>();
    std::vector<std::string> selected{};
    selected.reserve(feature_names_.size());
    for (std::size_t i = 0; i < feature_names_.size() && i < mask.size(); ++i) {
      if (mask[i] > 0.5) selected.push_back(feature_names_[i]);
    }
    return Plan(std::move(selected));
  }

 private:
  std::vector<std::string> feature_names_{};
  std::int64_t sample_count_{0};
  Series mean_ = Series::undefined();
  Series stddev_ = Series::undefined();
  Series minimum_ = Series::undefined();
  Series maximum_ = Series::undefined();
  Series skewness_ = Series::undefined();
  Series excess_kurtosis_ = Series::undefined();
  Series missing_rate_ = Series::undefined();
  Series negative_rate_ = Series::undefined();
  Series zero_rate_ = Series::undefined();
  Series gaussianize_candidate_ = Series::undefined();
};

class Runner final {
 public:
  Runner(FeatureBatch batch, std::vector<std::string> feature_names = {})
      : batch_(std::move(batch)), feature_names_(std::move(feature_names)) {}

  [[nodiscard]] Result run(ProfileOptions options = {}) const {
    if (!batch_.x.defined() || batch_.x.dim() != 2) {
      ::pluss::throw_error("shape::data(...).run requires a rank-2 FeatureBatch.");
    }
    const Series x = batch_.layout == ObservationLayout::Rows
                         ? batch_.x.to(ScalarType::Float64)
                         : batch_.x.to(ScalarType::Float64).transpose(0, 1);
    const std::int64_t n = x.sizes()[0];
    const std::int64_t p = x.sizes()[1];
    std::vector<std::string> names = feature_names_;
    if (names.empty()) {
      names.reserve(static_cast<std::size_t>(p));
      for (std::int64_t i = 0; i < p; ++i) names.push_back("x" + std::to_string(i));
    }
    if (static_cast<std::int64_t>(names.size()) != p) {
      ::pluss::throw_error("shape::data(...).run feature name count does not match feature count.");
    }

    const Series mean = x.mean(0, false).reshape({p});
    const Series centered = x.sub(mean.reshape({1, p}));
    const Series stddev = centered.square().mean(0, false).sqrt().reshape({p});
    const Series safe_std = stddev.clamp_min(options.low_variance_threshold).reshape({p});
    const Series z = centered.div(safe_std.reshape({1, p}));
    const Series skewness = z.pow(3.0).mean(0, false).reshape({p});
    const Series excess_kurtosis = z.pow(4.0).mean(0, false).sub(3.0).reshape({p});
    const Series missing_rate = x.isnan().to(ScalarType::Float64).mean(0, false).reshape({p});
    const Series negative_rate = x.lt(0.0).to(ScalarType::Float64).mean(0, false).reshape({p});
    const Series zero_rate = x.eq(0.0).to(ScalarType::Float64).mean(0, false).reshape({p});
    const Series candidate = stddev.gt(options.low_variance_threshold)
                                 .logical_and(skewness.abs().gt(options.skew_threshold)
                                                  .logical_or(excess_kurtosis.abs().gt(options.heavy_tail_threshold)))
                                 .to(ScalarType::Float64)
                                 .reshape({p});

    return Result(std::move(names),
                  n,
                  mean,
                  stddev,
                  x.amin(0, false).reshape({p}),
                  x.amax(0, false).reshape({p}),
                  skewness,
                  excess_kurtosis,
                  missing_rate,
                  negative_rate,
                  zero_rate,
                  candidate);
  }

 private:
  FeatureBatch batch_{};
  std::vector<std::string> feature_names_{};
};

[[nodiscard]] inline std::vector<std::string> default_feature_names_(std::int64_t feature_count) {
  std::vector<std::string> names{};
  names.reserve(static_cast<std::size_t>(feature_count));
  for (std::int64_t i = 0; i < feature_count; ++i) names.push_back("x" + std::to_string(i));
  return names;
}

[[nodiscard]] inline std::vector<std::string> feature_names_from_(const pluss::table::DataFrame& frame,
                                                                  const DataFrameBatchOptions& options) {
  std::vector<std::string> names = options.feature_names.empty() ? frame.features() : options.feature_names;
  if (options.label_feature.has_value()) {
    names.erase(std::remove(names.begin(), names.end(), *options.label_feature), names.end());
  }
  if (options.sample_weight_feature.has_value()) {
    names.erase(std::remove(names.begin(), names.end(), *options.sample_weight_feature), names.end());
  }
  return names;
}

[[nodiscard]] inline DataFrameBatchOptions default_data_options_(pluss::msharp::RangeView::Of<std::string> names) {
  DataFrameBatchOptions options{};
  options.feature_names = copy_range_values(names);
  options.layout = ObservationLayout::Rows;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

[[nodiscard]] inline Runner data(const FeatureBatch& batch, std::vector<std::string> feature_names = {}) {
  return Runner(batch, std::move(feature_names));
}

[[nodiscard]] inline Runner data(const pluss::table::DataFrame& frame, const DataFrameBatchOptions& options) {
  std::vector<std::string> names = feature_names_from_(frame, options);
  const FeatureBatch batch = resolve(feature_batch_from_dataframe(frame, options), "shape::data.feature_batch_from_dataframe");
  return Runner(batch, std::move(names));
}

[[nodiscard]] inline Runner data(const pluss::table::DataFrame& x) {
  const std::vector<std::string> names = x.feataxis().series().features();
  if (names.empty()) {
    ::pluss::throw_error("shape::data(DataFrame): input DataFrame has no Series features.");
  }
  return data(x, default_data_options_(names));
}

} // namespace pluss::ai::lymph::shape
