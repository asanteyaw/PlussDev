#pragma once

#include "pluss/ai/nodej/nodej.h"
#include "pluss/codem/series_factory.h"
#include "table/DataFrame.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::ai::lymph {

using namespace ::pluss::ai::nodej;


template <class T>
T resolve(StatusOr<T> value_or, std::string_view where = {}) {
  if (!value_or.ok()) {
    const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
    throw std::runtime_error(prefix + value_or.status().message);
  }
  return value_or.move_value();
}

inline void resolve(Status status, std::string_view where = {}) {
  if (status.ok()) return;
  const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
  throw std::runtime_error(prefix + status.message);
}

class WorkflowModel;

namespace prep {

using StandardScalerOptions = ::pluss::ai::nodej::prep::StandardScalerOptions;
using MinMaxScalerOptions = ::pluss::ai::nodej::prep::MinMaxScalerOptions;
using RobustScalerOptions = ::pluss::ai::nodej::prep::RobustScalerOptions;
using SimpleImputerStrategy = ::pluss::ai::nodej::prep::SimpleImputerStrategy;
using SimpleImputerOptions = ::pluss::ai::nodej::prep::SimpleImputerOptions;
using CategoryOverflowPolicy = ::pluss::ai::nodej::prep::CategoryOverflowPolicy;
using OneHotEncoderOptions = ::pluss::ai::nodej::prep::OneHotEncoderOptions;
using FrequencyEncoderOptions = ::pluss::ai::nodej::prep::FrequencyEncoderOptions;
using NeighborImputerWeighting = ::pluss::ai::nodej::prep::NeighborImputerWeighting;
using NeighborImputerOptions = ::pluss::ai::nodej::prep::NeighborImputerOptions;
using IterativeImputerOptions = ::pluss::ai::nodej::prep::IterativeImputerOptions;
using VectorNorm = ::pluss::ai::nodej::prep::VectorNorm;
using VectorNormalizerOptions = ::pluss::ai::nodej::prep::VectorNormalizerOptions;
using MaxAbsScalerOptions = ::pluss::ai::nodej::prep::MaxAbsScalerOptions;
using QuantileMapOutput = ::pluss::ai::nodej::prep::QuantileMapOutput;
using QuantileMapOptions = ::pluss::ai::nodej::prep::QuantileMapOptions;
using PowerMapMode = ::pluss::ai::nodej::prep::PowerMapMode;
using PowerMapOptions = ::pluss::ai::nodej::prep::PowerMapOptions;
using ThresholdBinarizerOptions = ::pluss::ai::nodej::prep::ThresholdBinarizerOptions;
using PolynomialLiftOptions = ::pluss::ai::nodej::prep::PolynomialLiftOptions;

struct SimpleImputerSummary {
  SimpleImputerStrategy strategy = SimpleImputerStrategy::Mean;
  bool learned = false;
  std::int64_t feature_count = 0;
  double constant_value = 0.0;
  double empty_feature_fallback = 0.0;
  std::vector<std::string> feature_names{};
  Series statistics = Series::undefined();
};

struct StandardScalerSummary {
  std::vector<std::string> feature_names{};
  Series mean = Series::undefined();
  Series scale = Series::undefined();
};

struct TransformerSummary {
  std::vector<std::string> feature_names{};
  std::vector<Series> state{};
};

inline const char* simple_imputer_strategy_name(SimpleImputerStrategy strategy) {
  switch (strategy) {
    case SimpleImputerStrategy::Mean:
      return "mean";
    case SimpleImputerStrategy::Median:
      return "median";
    case SimpleImputerStrategy::MostFrequent:
      return "most_frequent";
    case SimpleImputerStrategy::Constant:
      return "constant";
    default:
      return "unknown";
  }
}

inline void output_render(std::ostream& os, const SimpleImputerSummary& summary) {
  os << "SimpleImputerSummary{strategy=" << simple_imputer_strategy_name(summary.strategy)
     << ", learned=" << (summary.learned ? "true" : "false")
     << ", feature_count=" << summary.feature_count
     << ", constant_value=" << summary.constant_value
     << ", empty_feature_fallback=" << summary.empty_feature_fallback
     << ", feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], statistics=" << summary.statistics << "}";
}

inline void output_render(std::ostream& os, const StandardScalerSummary& summary) {
  os << "StandardScalerSummary{feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], mean=" << summary.mean << ", scale=" << summary.scale << "}";
}

inline void output_render(std::ostream& os, const TransformerSummary& summary) {
  os << "TransformerSummary{feature_names=[";
  for (std::size_t i = 0; i < summary.feature_names.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.feature_names[i];
  }
  os << "], state=[";
  for (std::size_t i = 0; i < summary.state.size(); ++i) {
    if (i != 0) os << ", ";
    os << summary.state[i];
  }
  os << "]}";
}

inline std::ostream& operator<<(std::ostream& os, const SimpleImputerSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const StandardScalerSummary& summary) {
  output_render(os, summary);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const TransformerSummary& summary) {
  output_render(os, summary);
  return os;
}

class SimpleImputer final {
public:
  explicit SimpleImputer(SimpleImputerOptions options = {})
      : options_(options), core_(options) {}

  SimpleImputer& features(std::initializer_list<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }

  SimpleImputer& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,
                                                  const TrainOptions& learn_options = {},
                                                  const InferOptions& infer_options = {});

  [[nodiscard]] SimpleImputerSummary summary() const {
    return SimpleImputerSummary{
        .strategy = options_.strategy,
        .learned = learned_,
        .feature_count = static_cast<std::int64_t>(feature_names_.size()),
        .constant_value = options_.constant_value,
        .empty_feature_fallback = options_.empty_feature_fallback,
        .feature_names = feature_names_,
        .statistics = statistics_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  SimpleImputerOptions options_{};
  std::vector<std::string> feature_names_{};
  Series statistics_ = Series::undefined();

private:
  [[nodiscard]] std::vector<std::string> resolved_feature_names_(const pluss::table::DataFrame& frame, const char* where) const;
  [[nodiscard]] static DataFrameBatchOptions batch_options_for_(const std::vector<std::string>& names);
  [[nodiscard]] static pluss::table::DataFrame merge_mapped_(const pluss::table::DataFrame& frame,
                                                             const std::vector<std::string>& names,
                                                             const Series& mapped,
                                                             const char* where);
  void refresh_statistics_();
  ::pluss::ai::nodej::prep::SimpleImputer core_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

class StandardScaler final {
public:
  explicit StandardScaler(StandardScalerOptions options = {})
      : options_(options), core_(options) {}

  StandardScaler& features(std::initializer_list<std::string_view> names) {
    feature_names_.clear();
    feature_names_.reserve(names.size());
    for (auto name : names) {
      const std::string owned(name);
      if (!owned.empty()) feature_names_.push_back(owned);
    }
    feature_names_pinned_ = !feature_names_.empty();
    return *this;
  }

  StandardScaler& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {});
  [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame, const InferOptions& options = {}) const;
  [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,
                                                  const TrainOptions& learn_options = {},
                                                  const InferOptions& infer_options = {});

  [[nodiscard]] StandardScalerSummary summary() const {
    return StandardScalerSummary{
        .feature_names = feature_names_,
        .mean = mean_,
        .scale = scale_,
    };
  }

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  StandardScalerOptions options_{};
  std::vector<std::string> feature_names_{};
  Series mean_ = Series::undefined();
  Series scale_ = Series::undefined();

private:
  [[nodiscard]] std::vector<std::string> resolved_feature_names_(const pluss::table::DataFrame& frame, const char* where) const;
  [[nodiscard]] static DataFrameBatchOptions batch_options_for_(const std::vector<std::string>& names);
  [[nodiscard]] static pluss::table::DataFrame merge_mapped_(const pluss::table::DataFrame& frame,
                                                             const std::vector<std::string>& names,
                                                             const Series& mapped,
                                                             const char* where);
  void refresh_statistics_();
  ::pluss::ai::nodej::prep::StandardScaler core_{};
  bool feature_names_pinned_{false};
  bool learned_{false};
};

#define PLUSS_NODEJ_DECLARE_PREP_WRAPPER(WrapperName, OptionsType)                              \
  class WrapperName final {                                                                      \
  public:                                                                                        \
    explicit WrapperName(OptionsType options = {})                                               \
        : options_(options), core_(options) {}                                                   \
                                                                                                 \
    WrapperName& features(std::initializer_list<std::string_view> names) {                      \
      feature_names_.clear();                                                                    \
      feature_names_.reserve(names.size());                                                      \
      for (auto name : names) {                                                                  \
        const std::string owned(name);                                                           \
        if (!owned.empty()) feature_names_.push_back(owned);                                     \
      }                                                                                          \
      feature_names_pinned_ = !feature_names_.empty();                                           \
      return *this;                                                                              \
    }                                                                                            \
                                                                                                 \
    WrapperName& learn(const pluss::table::DataFrame& frame, const TrainOptions& options = {}); \
    [[nodiscard]] pluss::table::DataFrame map(const pluss::table::DataFrame& frame,             \
                                              const InferOptions& options = {}) const;           \
    [[nodiscard]] pluss::table::DataFrame learn_map(const pluss::table::DataFrame& frame,       \
                                                    const TrainOptions& learn_options = {},      \
                                                    const InferOptions& infer_options = {});     \
                                                                                                 \
    [[nodiscard]] TransformerSummary summary() const {                                            \
      return TransformerSummary{                                                                  \
          .feature_names = feature_names_,                                                       \
          .state = state_,                                                                       \
      };                                                                                         \
    }                                                                                            \
                                                                                                 \
    [[nodiscard]] bool learned() const noexcept { return learned_; }                             \
    OptionsType options_{};                                                                      \
    std::vector<std::string> feature_names_{};                                                   \
    std::vector<Series> state_{};                                                                \
                                                                                                 \
  private:                                                                                       \
    [[nodiscard]] std::vector<std::string> resolved_feature_names_(                              \
        const pluss::table::DataFrame& frame,                                                    \
        const char* where) const;                                                                \
    [[nodiscard]] static DataFrameBatchOptions batch_options_for_(const std::vector<std::string>& names); \
    [[nodiscard]] static pluss::table::DataFrame merge_mapped_(                                  \
        const pluss::table::DataFrame& frame,                                                    \
        const std::vector<std::string>& names,                                                   \
        const Series& mapped,                                                                    \
        const char* where);                                                                      \
    void refresh_state_();                                                                       \
    ::pluss::ai::nodej::prep::WrapperName core_{};                                               \
    bool feature_names_pinned_{false};                                                           \
    bool learned_{false};                                                                        \
  };

PLUSS_NODEJ_DECLARE_PREP_WRAPPER(MinMaxScaler, MinMaxScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(RobustScaler, RobustScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(NeighborImputer, NeighborImputerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(IterativeImputer, IterativeImputerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(OneHotEncoder, OneHotEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(FrequencyEncoder, FrequencyEncoderOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(VectorNormalizer, VectorNormalizerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(MaxAbsScaler, MaxAbsScalerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(QuantileMap, QuantileMapOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(PowerMap, PowerMapOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(ThresholdBinarizer, ThresholdBinarizerOptions)
PLUSS_NODEJ_DECLARE_PREP_WRAPPER(PolynomialLift, PolynomialLiftOptions)

#undef PLUSS_NODEJ_DECLARE_PREP_WRAPPER

[[nodiscard]] inline SimpleImputer simple_imputer(SimpleImputerOptions options = {}) { return SimpleImputer(options); }
[[nodiscard]] inline StandardScaler standard_scaler(StandardScalerOptions options = {}) { return StandardScaler(options); }
[[nodiscard]] inline MinMaxScaler minmax_scaler(MinMaxScalerOptions options = {}) { return MinMaxScaler(options); }
[[nodiscard]] inline RobustScaler robust_scaler(RobustScalerOptions options = {}) { return RobustScaler(options); }
[[nodiscard]] inline NeighborImputer neighbor_imputer(NeighborImputerOptions options = {}) { return NeighborImputer(options); }
[[nodiscard]] inline IterativeImputer iterative_imputer(IterativeImputerOptions options = {}) { return IterativeImputer(options); }
[[nodiscard]] inline OneHotEncoder one_hot_encoder(OneHotEncoderOptions options = {}) { return OneHotEncoder(options); }
[[nodiscard]] inline FrequencyEncoder frequency_encoder(FrequencyEncoderOptions options = {}) { return FrequencyEncoder(options); }
[[nodiscard]] inline VectorNormalizer vector_normalizer(VectorNormalizerOptions options = {}) { return VectorNormalizer(options); }
[[nodiscard]] inline MaxAbsScaler maxabs_scaler(MaxAbsScalerOptions options = {}) { return MaxAbsScaler(options); }
[[nodiscard]] inline QuantileMap quantile_map(QuantileMapOptions options = {}) { return QuantileMap(options); }
[[nodiscard]] inline PowerMap power_map(PowerMapOptions options = {}) { return PowerMap(options); }
[[nodiscard]] inline ThresholdBinarizer threshold_binarizer(ThresholdBinarizerOptions options = {}) {
  return ThresholdBinarizer(options);
}
[[nodiscard]] inline PolynomialLift polynomial_lift(PolynomialLiftOptions options = {}) { return PolynomialLift(options); }

} // namespace prep

struct MetricReport {
  std::vector<MetricKind> requested{};
  Series values = Series::undefined();
  Series accuracy = Series::undefined();
  Series balanced_accuracy = Series::undefined();
  Series precision = Series::undefined();
  Series recall = Series::undefined();
  Series f1 = Series::undefined();
  Series roc_auc = Series::undefined();
  Series logloss = Series::undefined();
  Series mse = Series::undefined();
  Series rmse = Series::undefined();
  Series mae = Series::undefined();
  Series r2 = Series::undefined();
  Series quantile_loss = Series::undefined();
};

struct SeriesCompareReport {
  Series abs_error = Series::undefined();
  Series max_abs_diff = Series::undefined();
  Series mae = Series::undefined();
};

[[nodiscard]] inline SeriesCompareReport compare_series(const Series& lhs, const Series& rhs) {
  const Series a = lhs.to(pluss::pCPU, ScalarType::Float64).reshape({lhs.numel()});
  const Series b = rhs.to(pluss::pCPU, ScalarType::Float64).reshape({rhs.numel()});
  if (a.numel() != b.numel()) {
    throw std::runtime_error("compare_series: shape mismatch.");
  }
  const Series abs = a.sub(b).abs();
  return SeriesCompareReport{
      .abs_error = abs,
      .max_abs_diff = abs.amax().reshape({1}),
      .mae = abs.mean().reshape({1}),
  };
}

[[nodiscard]] inline double max_abs_diff(const Series& lhs, const Series& rhs) {
  return compare_series(lhs, rhs).max_abs_diff.item<double>();
}

struct LinearParameters {
  Series coeff = Series::undefined();
  Series intercept = Series::undefined();
  bool fit_intercept = false;
};

struct HoldoutSplitOptions {
  double test_size = 0.2;
  Series stratify = Series::undefined();
  std::string stratify_feature{};
  std::uint64_t random_state = 0;
  bool deterministic = true;
  bool shuffle = true;
};

struct FrameHoldoutSplit {
  pluss::table::DataFrame train_set{};
  pluss::table::DataFrame test_set{};
  IndexSplit indices{};
};

struct BatchHoldoutSplit {
  LabeledBatch train_set{};
  LabeledBatch test_set{};
  IndexSplit indices{};
};

[[nodiscard]] inline Series as_stratify_series(const pluss::table::Thread& thread);

struct CrossValidationOptions {
  CVPlanKind plan = CVPlanKind::KFold;
  double holdout_fraction = 0.2;
  std::int64_t folds = 5;
  bool stratified = false;
  bool reliability_defaults = true;
  bool shuffle = true;
  std::uint64_t random_state = 0;
  bool deterministic = true;
};

struct CrossValidationReport {
  CrossAssessReport report{};
  MetricReport mean{};
  MetricReport stdev{};
};

[[nodiscard]] inline SplitOptions make_holdout_split_options(const HoldoutSplitOptions& options) {
  SplitOptions split{};
  split.holdout_fraction = options.test_size;
  split.shuffle = options.shuffle;
  split.random.seed = options.random_state;
  split.random.deterministic = options.deterministic;
  return split;
}

[[nodiscard]] inline SplitOptions make_cross_validation_split_options(const CrossValidationOptions& options) {
  SplitOptions split{};
  split.plan = options.plan;
  split.holdout_fraction = options.holdout_fraction;
  split.folds = options.folds;
  split.stratified = options.stratified;
  split.reliability_defaults = options.reliability_defaults;
  split.shuffle = options.shuffle;
  split.random.seed = options.random_state;
  split.random.deterministic = options.deterministic;
  return split;
}

[[nodiscard]] inline Series metric_scalar_(double value) {
  return pluss::make_series(std::vector<double>{value}, {1}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
}

[[nodiscard]] inline MetricReport metric_report_from_series_(const Series& metrics, const std::vector<MetricKind>& requested) {
  MetricReport report{};
  report.requested = requested;
  report.values = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()});
  const std::vector<double> values = report.values.to_vec<double>();

  const std::size_t n = std::min(values.size(), requested.size());
  for (std::size_t i = 0; i < n; ++i) {
    const Series scalar = metric_scalar_(values[i]);
    switch (requested[i]) {
      case MetricKind::Accuracy: report.accuracy = scalar; break;
      case MetricKind::BalancedAccuracy: report.balanced_accuracy = scalar; break;
      case MetricKind::Precision: report.precision = scalar; break;
      case MetricKind::Recall: report.recall = scalar; break;
      case MetricKind::F1: report.f1 = scalar; break;
      case MetricKind::RocAuc: report.roc_auc = scalar; break;
      case MetricKind::LogLoss: report.logloss = scalar; break;
      case MetricKind::Mse: report.mse = scalar; break;
      case MetricKind::Rmse: report.rmse = scalar; break;
      case MetricKind::Mae: report.mae = scalar; break;
      case MetricKind::R2: report.r2 = scalar; break;
      case MetricKind::QuantileLoss: report.quantile_loss = scalar; break;
    }
  }
  return report;
}

[[nodiscard]] inline std::vector<MetricKind> default_assess_metrics_(TaskKind task, const AssessOptions& options) {
  if (!options.metrics.empty()) return options.metrics;
  if (task == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  if (task == TaskKind::Classification) return {MetricKind::Accuracy};
  return {};
}

[[nodiscard]] inline std::vector<std::string> cross_validate_default_feature_names_(
    const pluss::table::DataFrame& frame,
    const char* where) {
  const auto names = frame.series_features().feature_names();
  if (names.empty()) {
    throw std::runtime_error(std::string(where) + ": input DataFrame has no Series features.");
  }
  return names;
}

[[nodiscard]] inline DataFrameBatchOptions cross_validate_default_data_options_(const std::vector<std::string>& names) {
  DataFrameBatchOptions options{};
  options.feature_names = names;
  options.layout = ObservationLayout::Rows;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

[[nodiscard]] inline LabeledBatch cross_validate_labeled_from_xy_(const pluss::table::DataFrame& x,
                                                                  const Series& y,
                                                                  const std::vector<std::string>& feature_names,
                                                                  const char* where) {
  const auto x_batch = resolve(feature_batch_from_dataframe(x, cross_validate_default_data_options_(feature_names)),
                               std::string(where) + ".feature_batch_from_dataframe");
  const Series y_aligned = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()});
  LabeledBatch batch{
      .features = x_batch,
      .y = y_aligned,
      .sample_weights = Series::undefined(),
  };
  resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
  return batch;
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const SplitOptions split = make_cross_validation_split_options(cv);
  CrossAssessReport report =
      resolve(pluss::ai::nodej::cross_assess(spec, batch, split, train_options, assess_options), "cross_assess");
  const auto metrics = default_assess_metrics_(spec.task, assess_options);
  return CrossValidationReport{
      .report = report,
      .mean = metric_report_from_series_(report.mean_metrics, metrics),
      .stdev = metric_report_from_series_(report.std_metrics, metrics),
  };
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& frame,
                                                          const DataFrameBatchOptions& data_options,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate.labeled_batch_from_dataframe");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(x,y)");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(spec, x, as_stratify_series(y), cv, train_options, assess_options);
}

[[nodiscard]] inline Series as_stratify_series(const pluss::table::Thread& thread) {
  if (thread.kind() == pluss::table::ThreadKind::Series) return thread.as_series();
  return thread.cast(pluss::table::ThreadCastTarget::Series, ScalarType::Float64).as_series();
}

[[nodiscard]] inline IndexSplit make_holdout_indices(const pluss::table::DataFrame& frame, const HoldoutSplitOptions& options) {
  const SplitOptions split = make_holdout_split_options(options);
  Series stratify = options.stratify;
  if (!stratify.defined() && !options.stratify_feature.empty()) {
    stratify = as_stratify_series(frame.features(options.stratify_feature));
  }
  if (stratify.defined()) {
    return resolve(pluss::ai::nodej::strat_holdout_split(stratify, split), "strat_holdout_split");
  }
  return resolve(pluss::ai::nodej::holdout_split(frame.ncases(), split), "holdout_split");
}

[[nodiscard]] inline FrameHoldoutSplit holdout_split(const pluss::table::DataFrame& frame,
                                                     HoldoutSplitOptions options = {}) {
  const IndexSplit indices = make_holdout_indices(frame, options);
  const std::vector<std::int64_t> train_idx = indices.train_indices.to(pluss::pCPU, ScalarType::Int64)
                                                   .reshape({indices.train_indices.numel()})
                                                   .to_vec<std::int64_t>();
  const std::vector<std::int64_t> test_idx = indices.test_indices.to(pluss::pCPU, ScalarType::Int64)
                                                 .reshape({indices.test_indices.numel()})
                                                 .to_vec<std::int64_t>();
  return FrameHoldoutSplit{
      .train_set = frame.take(train_idx),
      .test_set = frame.take(test_idx),
      .indices = indices,
  };
}

[[nodiscard]] inline BatchHoldoutSplit holdout_split(const LabeledBatch& batch,
                                                     HoldoutSplitOptions options = {}) {
  const SplitOptions split = make_holdout_split_options(options);
  const bool use_stratify = options.stratify.defined();
  const IndexSplit indices = use_stratify
                                 ? resolve(pluss::ai::nodej::strat_holdout_split(options.stratify, split), "strat_holdout_split")
                                 : resolve(pluss::ai::nodej::holdout_split(batch.features.sample_count(), split), "holdout_split");
  return BatchHoldoutSplit{
      .train_set = resolve(pluss::ai::nodej::slice_labeled_batch(batch, indices.train_indices), "slice_labeled_batch(train)"),
      .test_set = resolve(pluss::ai::nodej::slice_labeled_batch(batch, indices.test_indices), "slice_labeled_batch(test)"),
      .indices = indices,
  };
}

[[nodiscard]] inline std::pair<pluss::table::DataFrame, pluss::table::DataFrame> hold_out_split(
    const pluss::table::DataFrame& frame,
    HoldoutSplitOptions options = {}) {
  auto split = holdout_split(frame, std::move(options));
  return std::pair<pluss::table::DataFrame, pluss::table::DataFrame>(std::move(split.train_set), std::move(split.test_set));
}

class WorkflowModel final {
public:
  explicit WorkflowModel(const ModelSpec& spec)
      : workflow_(resolve(make_flow(spec), "make_flow")) {
    refresh_linear_summary_();
  }

  explicit WorkflowModel(Workflow workflow)
      : workflow_(std::move(workflow)) {
    refresh_linear_summary_();
  }

  // Exposed linear summary for sklearn-like convenience.
  Series coeff = Series::undefined();
  Series intercept = Series::undefined();

  [[nodiscard]] bool trained() const noexcept { return workflow_.trained(); }
  [[nodiscard]] bool has_model() const noexcept { return workflow_.has_model(); }
  [[nodiscard]] bool has_model_spec() const noexcept { return workflow_.has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const noexcept { return workflow_.has_opaque_transforms(); }
  [[nodiscard]] std::int64_t stage_count() const noexcept { return workflow_.stage_count(); }
  [[nodiscard]] TaskKind task_kind() const noexcept { return workflow_.task_kind(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const noexcept { return workflow_.model_spec(); }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const noexcept { return workflow_.transform_specs(); }

  WorkflowModel& standard_scaler(prep::StandardScalerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_standard_scaler(options, std::move(feature_route)), "standard_scaler");
    return *this;
  }

  WorkflowModel& minmax_scaler(prep::MinMaxScalerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_minmax_scaler(options, std::move(feature_route)), "minmax_scaler");
    return *this;
  }

  WorkflowModel& robust_scaler(prep::RobustScalerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_robust_scaler(options, std::move(feature_route)), "robust_scaler");
    return *this;
  }

  WorkflowModel& simple_imputer(prep::SimpleImputerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_simple_imputer(options, std::move(feature_route)), "simple_imputer");
    return *this;
  }

  WorkflowModel& neighbor_imputer(prep::NeighborImputerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_neighbor_imputer(options, std::move(feature_route)), "neighbor_imputer");
    return *this;
  }

  WorkflowModel& iterative_imputer(prep::IterativeImputerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_iterative_imputer(options, std::move(feature_route)), "iterative_imputer");
    return *this;
  }

  WorkflowModel& one_hot_encoder(prep::OneHotEncoderOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_one_hot_encoder(options, std::move(feature_route)), "one_hot_encoder");
    return *this;
  }

  WorkflowModel& frequency_encoder(prep::FrequencyEncoderOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_frequency_encoder(options, std::move(feature_route)), "frequency_encoder");
    return *this;
  }

  WorkflowModel& vector_normalizer(prep::VectorNormalizerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_vector_normalizer(options, std::move(feature_route)), "vector_normalizer");
    return *this;
  }

  WorkflowModel& maxabs_scaler(prep::MaxAbsScalerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_maxabs_scaler(options, std::move(feature_route)), "maxabs_scaler");
    return *this;
  }

  WorkflowModel& quantile_map(prep::QuantileMapOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_quantile_map(options, std::move(feature_route)), "quantile_map");
    return *this;
  }

  WorkflowModel& power_map(prep::PowerMapOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_power_map(options, std::move(feature_route)), "power_map");
    return *this;
  }

  WorkflowModel& threshold_binarizer(prep::ThresholdBinarizerOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_threshold_binarizer(options, std::move(feature_route)), "threshold_binarizer");
    return *this;
  }

  WorkflowModel& polynomial_lift(prep::PolynomialLiftOptions options = {}, std::vector<std::int64_t> feature_route = {}) {
    resolve(workflow_.add_polynomial_lift(options, std::move(feature_route)), "polynomial_lift");
    return *this;
  }

  WorkflowModel& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(workflow_.learn(batch, options), "learn");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& frame,
                        const DataFrameBatchOptions& data_options,
                        const TrainOptions& options = {}) {
    resolve(workflow_.learn(frame, data_options, options), "learn");
    feature_names_ = resolve_feature_names_from_options_(frame, data_options, "learn_");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& x,
                        const Series& y,
                        const TrainOptions& options = {}) {
    const auto names = default_series_feature_names_(x, "learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& x,
                        const pluss::table::Thread& y,
                        const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] WorkflowModel learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(batch, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(frame, data_options, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.map(batch, options), "map");
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    const auto batch = resolve(feature_batch_from_dataframe(frame, data_options), "map.feature_batch_from_dataframe");
    return map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& x,
                           const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "map(x)");
    return map(x, default_data_options_(names), options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    Series mapped = resolve(workflow_.learn_map(batch, learn_options, infer_options), "learn_map");
    refresh_linear_summary_();
    return mapped;
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    const auto batch = resolve(feature_batch_from_dataframe(frame, data_options), "learn_map.feature_batch_from_dataframe");
    const Series mapped = learn_map(batch, learn_options, infer_options);
    feature_names_ = resolve_feature_names_from_options_(frame, data_options, "learn_map");
    return mapped;
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& x,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "learn_map(x)");
    return learn_map(x, default_data_options_(names), learn_options, infer_options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.infer(batch, options), "infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return resolve(workflow_.infer(frame, data_options, options), "infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer(x)");
    return resolve(workflow_.infer(x, default_data_options_(names), options), "infer");
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.infer_scores(batch, options), "infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    return resolve(workflow_.infer_scores(frame, data_options, options), "infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x,
                                    const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer_scores(x)");
    return resolve(workflow_.infer_scores(x, default_data_options_(names), options), "infer_scores");
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(workflow_.assess(batch, options), "assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    return resolve(workflow_.assess(frame, data_options, options), "assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "assess(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "assess(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(batch, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(frame, data_options, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(x, y, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(x, y, options), resolved_metrics_(options));
  }

  [[nodiscard]] LinearParameters linear_parameters() const {
    const auto& spec_opt = workflow_.model_spec();
    if (!spec_opt.has_value()) {
      throw std::runtime_error("linear_parameters: workflow has no model specification.");
    }
    if (spec_opt->family != ModelFamily::Linear) {
      throw std::runtime_error("linear_parameters: only available for linear models.");
    }

    const auto* linear_options = std::get_if<LinearFamilyOptions>(&spec_opt->options);
    if (linear_options == nullptr) {
      throw std::runtime_error("linear_parameters: model options do not contain LinearFamilyOptions.");
    }
    const bool fit_intercept = linear_options->fit_intercept;

    const std::vector<Series> state = resolve(workflow_.dump_model_state(), "dump_model_state");
    if (state.size() < 2 || !state[1].defined()) {
      throw std::runtime_error("linear_parameters: missing coefficient state.");
    }

    const std::vector<double> beta =
        state[1].to(pluss::pCPU, ScalarType::Float64).reshape({state[1].numel()}).to_vec<double>();
    if (beta.empty()) {
      throw std::runtime_error("linear_parameters: coefficient vector is empty.");
    }

    LinearParameters out{};
    out.fit_intercept = fit_intercept;
    if (fit_intercept) {
      out.intercept = scalar_series_(beta.front());
      out.coeff = vector_series_(std::vector<double>(beta.begin() + 1, beta.end()));
    } else {
      out.intercept = scalar_series_(0.0);
      out.coeff = vector_series_(beta);
    }
    return out;
  }

  [[nodiscard]] std::vector<double> coefficients() const {
    if (!coeff.defined()) return {};
    return coeff.to(pluss::pCPU, ScalarType::Float64).reshape({coeff.numel()}).to_vec<double>();
  }

  [[nodiscard]] double intercept_term() const {
    if (!intercept.defined() || intercept.numel() == 0) return 0.0;
    return intercept.to(pluss::pCPU, ScalarType::Float64).reshape({intercept.numel()}).item<double>();
  }

  void save(const std::string& path,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    resolve(io::save(path, workflow_, train_batch, train_options, std::move(metadata)), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x_train, "save(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x_train, y_train, names, "save(x,y)");
    resolve(io::save(path, workflow_, batch, train_options, std::move(metadata)), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    save(path, x_train, as_stratify_series(y_train), train_options, std::move(metadata));
  }

  [[nodiscard]] static WorkflowModel restore(const io::FlowBundle& bundle) {
    return WorkflowModel(resolve(io::restore(bundle), "restore"));
  }

  [[nodiscard]] static WorkflowModel load(const std::string& path) {
    return WorkflowModel(resolve(io::load_and_restore(path), "load_and_restore"));
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    return resolve(workflow_.dump_transform_states(), "dump_transform_states");
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    return resolve(workflow_.dump_model_state(), "dump_model_state");
  }

  WorkflowModel& load_transform_states_(const std::vector<std::vector<Series>>& states) {
    resolve(workflow_.load_transform_states(states), "load_transform_states");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& load_model_state_(const std::vector<Series>& state) {
    resolve(workflow_.load_model_state(state), "load_model_state");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& mark_trained_(bool trained = true) {
    workflow_.mark_trained(trained);
    if (!trained) {
      coeff = Series::undefined();
      intercept = Series::undefined();
    } else {
      refresh_linear_summary_();
    }
    return *this;
  }

  [[nodiscard]] const Workflow& workflow() const noexcept { return workflow_; }
  [[nodiscard]] Workflow& workflow() noexcept { return workflow_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(const std::vector<std::string>& feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = feature_names;
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.series_features().feature_names();
    if (names.empty()) {
      throw std::runtime_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static std::vector<std::string> resolve_feature_names_from_options_(
      const pluss::table::DataFrame& frame,
      const DataFrameBatchOptions& options,
      const char* where) {
    std::vector<std::string> names = options.feature_names.empty() ? frame.feature_names() : options.feature_names;
    if (options.label_feature.has_value()) {
      names.erase(std::remove(names.begin(), names.end(), *options.label_feature), names.end());
    }
    if (options.sample_weight_feature.has_value()) {
      names.erase(std::remove(names.begin(), names.end(), *options.sample_weight_feature), names.end());
    }
    if (names.empty()) {
      throw std::runtime_error(std::string(where) + ": resolved zero feature names.");
    }
    return names;
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     const std::vector<std::string>& feature_names,
                                                     const char* where) {
    const auto x_batch =
        resolve(feature_batch_from_dataframe(x, default_data_options_(feature_names)), std::string(where) + ".feature_batch_from_dataframe");
    const Series y_aligned = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()});
    LabeledBatch batch{
        .features = x_batch,
        .y = y_aligned,
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  [[nodiscard]] static Status append_transform_from_spec_(Workflow& flow, const TransformSpec& spec) {
    switch (spec.kind) {
      case TransformKind::StandardScaler:
        return flow.add_standard_scaler(std::get<prep::StandardScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::MinMaxScaler:
        return flow.add_minmax_scaler(std::get<prep::MinMaxScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::RobustScaler:
        return flow.add_robust_scaler(std::get<prep::RobustScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::SimpleImputer:
        return flow.add_simple_imputer(std::get<prep::SimpleImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::NeighborImputer:
        return flow.add_neighbor_imputer(std::get<prep::NeighborImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::IterativeImputer:
        return flow.add_iterative_imputer(std::get<prep::IterativeImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::OneHotEncoder:
        return flow.add_one_hot_encoder(std::get<prep::OneHotEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::FrequencyEncoder:
        return flow.add_frequency_encoder(std::get<prep::FrequencyEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::VectorNormalizer:
        return flow.add_vector_normalizer(std::get<prep::VectorNormalizerOptions>(spec.options), spec.feature_route);
      case TransformKind::MaxAbsScaler:
        return flow.add_maxabs_scaler(std::get<prep::MaxAbsScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::QuantileMap:
        return flow.add_quantile_map(std::get<prep::QuantileMapOptions>(spec.options), spec.feature_route);
      case TransformKind::PowerMap:
        return flow.add_power_map(std::get<prep::PowerMapOptions>(spec.options), spec.feature_route);
      case TransformKind::ThresholdBinarizer:
        return flow.add_threshold_binarizer(std::get<prep::ThresholdBinarizerOptions>(spec.options), spec.feature_route);
      case TransformKind::PolynomialLift:
        return flow.add_polynomial_lift(std::get<prep::PolynomialLiftOptions>(spec.options), spec.feature_route);
    }
    return Status::internal("append_transform_from_spec: unknown transform kind.");
  }

  [[nodiscard]] WorkflowModel clone_for_out_of_place_() const {
    if (workflow_.has_opaque_transforms()) {
      throw std::runtime_error("learn(out-of-place): workflow has opaque transforms that cannot be cloned.");
    }
    if (!workflow_.model_spec().has_value() && workflow_.has_model()) {
      throw std::runtime_error("learn(out-of-place): workflow model has no clonable ModelSpec.");
    }

    ::pluss::ai::nodej::Workflow cloned =
        workflow_.model_spec().has_value() ? resolve(make_flow(*workflow_.model_spec()), "make_flow(clone)")
                                           : ::pluss::ai::nodej::Workflow{};
    for (const auto& spec : workflow_.transform_specs()) {
      resolve(append_transform_from_spec_(cloned, spec), "append_transform_from_spec(clone)");
    }

    if (workflow_.trained()) {
      resolve(cloned.load_transform_states(resolve(workflow_.dump_transform_states(), "dump_transform_states(clone)")),
              "load_transform_states(clone)");
      if (cloned.has_model()) {
        resolve(cloned.load_model_state(resolve(workflow_.dump_model_state(), "dump_model_state(clone)")),
                "load_model_state(clone)");
      }
      cloned.mark_trained(true);
    }
    WorkflowModel out(std::move(cloned));
    out.feature_names_ = feature_names_;
    return out;
  }

  [[nodiscard]] static Series scalar_series_(double value) {
    return pluss::make_series(std::vector<double>{value}, {1}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  }

  [[nodiscard]] static Series vector_series_(std::vector<double> values) {
    const auto n = static_cast<std::int64_t>(values.size());
    return pluss::make_series(
        std::move(values),
        {n},
        Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  }

  [[nodiscard]] std::vector<MetricKind> resolved_metrics_(const AssessOptions& options) const {
    if (!options.metrics.empty()) return options.metrics;
    if (workflow_.task_kind() == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    if (workflow_.task_kind() == TaskKind::Classification) return {MetricKind::Accuracy};
    return {};
  }

  [[nodiscard]] static MetricReport as_metric_report_(const Series& metrics, const std::vector<MetricKind>& requested) {
    MetricReport report{};
    report.requested = requested;
    report.values = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()});
    const std::vector<double> values = report.values.to_vec<double>();

    const std::size_t n = std::min(values.size(), requested.size());
    for (std::size_t i = 0; i < n; ++i) {
      const Series scalar = scalar_series_(values[i]);
      switch (requested[i]) {
        case MetricKind::Accuracy: report.accuracy = scalar; break;
        case MetricKind::BalancedAccuracy: report.balanced_accuracy = scalar; break;
        case MetricKind::Precision: report.precision = scalar; break;
        case MetricKind::Recall: report.recall = scalar; break;
        case MetricKind::F1: report.f1 = scalar; break;
        case MetricKind::RocAuc: report.roc_auc = scalar; break;
        case MetricKind::LogLoss: report.logloss = scalar; break;
        case MetricKind::Mse: report.mse = scalar; break;
        case MetricKind::Rmse: report.rmse = scalar; break;
        case MetricKind::Mae: report.mae = scalar; break;
        case MetricKind::R2: report.r2 = scalar; break;
        case MetricKind::QuantileLoss: report.quantile_loss = scalar; break;
      }
    }
    return report;
  }

  void refresh_linear_summary_() {
    coeff = Series::undefined();
    intercept = Series::undefined();
    try {
      const LinearParameters p = linear_parameters();
      coeff = p.coeff;
      intercept = p.intercept;
    } catch (...) {}
  }

  Workflow workflow_{};
  std::vector<std::string> feature_names_{};
};

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  resolve(validate_labeled(batch), "cross_validate(model).validate_labeled");
  if (!model.workflow().has_model()) {
    throw std::runtime_error("cross_validate(model,...): workflow has no model.");
  }

  const auto spec_opt = model.workflow().model_spec();
  const TaskKind task = spec_opt.has_value() ? spec_opt->task : model.workflow().task_kind();
  const std::vector<MetricKind> metrics = default_assess_metrics_(task, assess_options);
  if (metrics.empty()) {
    throw std::runtime_error("cross_validate(model,...): no assessment metrics requested.");
  }

  const auto make_plan = [&](const SplitOptions& split) -> SplitPlan {
    if (split.plan == CVPlanKind::Holdout) {
      const auto holdout = (split.stratified && task == TaskKind::Classification)
                               ? resolve(pluss::ai::nodej::strat_holdout_split(batch.y, split), "strat_holdout_split")
                               : resolve(pluss::ai::nodej::holdout_split(batch.features.sample_count(), split), "holdout_split");
      SplitPlan plan{};
      plan.folds.push_back(holdout);
      return plan;
    }
    if (split.plan == CVPlanKind::KFold) {
      if (split.stratified && task == TaskKind::Classification) {
        return resolve(pluss::ai::nodej::strat_fold_plan(batch.y, split), "strat_fold_plan");
      }
      return resolve(pluss::ai::nodej::fold_plan(batch.features.sample_count(), split), "fold_plan");
    }
    if (split.plan == CVPlanKind::StratifiedKFold) {
      if (task != TaskKind::Classification) {
        throw std::runtime_error("cross_validate(model,...): StratifiedKFold requires classification task.");
      }
      return resolve(pluss::ai::nodej::strat_fold_plan(batch.y, split), "strat_fold_plan");
    }
    throw std::runtime_error("cross_validate(model,...): only Holdout/KFold/StratifiedKFold are currently supported.");
  };

  SplitOptions split = make_cross_validation_split_options(cv);
  if (split.reliability_defaults && task == TaskKind::Classification
      && !split.stratified
      && (split.plan == CVPlanKind::Holdout || split.plan == CVPlanKind::KFold)) {
    split.stratified = true;
  }
  const SplitPlan plan = make_plan(split);
  if (plan.folds.empty()) {
    throw std::runtime_error("cross_validate(model,...): empty fold plan.");
  }

  CrossAssessReport report{};
  report.metrics = metrics;
  report.folds.reserve(plan.folds.size());
  AssessOptions fold_assess = assess_options;
  fold_assess.metrics = metrics;

  for (const auto& fold : plan.folds) {
    const LabeledBatch train_batch =
        resolve(pluss::ai::nodej::slice_labeled_batch(batch, fold.train_indices), "slice_labeled_batch(train)");
    const LabeledBatch test_batch =
        resolve(pluss::ai::nodej::slice_labeled_batch(batch, fold.test_indices), "slice_labeled_batch(test)");
    const WorkflowModel trained = model.learn(train_batch, train_options);
    const Series fold_metrics = trained.assess_series(test_batch, fold_assess).to(ScalarType::Float64).reshape({static_cast<std::int64_t>(metrics.size())});
    report.folds.push_back(FoldAssessResult{
        .metrics = fold_metrics,
        .train_indices = fold.train_indices,
        .test_indices = fold.test_indices,
    });
  }

  const std::int64_t fold_count = static_cast<std::int64_t>(report.folds.size());
  const std::int64_t metric_count = static_cast<std::int64_t>(metrics.size());
  Series fold_matrix = report.folds.front().metrics.reshape({1, metric_count});
  for (std::size_t i = 1; i < report.folds.size(); ++i) {
    fold_matrix = fold_matrix.unite({report.folds[i].metrics.reshape({1, metric_count})}, 0);
  }
  fold_matrix = fold_matrix.reshape({fold_count, metric_count});
  report.mean_metrics = fold_matrix.mean(0, false).reshape({metric_count});
  const Series centered = fold_matrix.sub(report.mean_metrics.reshape({1, metric_count}));
  report.std_metrics = centered.square().mean(0, false).sqrt().reshape({metric_count});

  return CrossValidationReport{
      .report = report,
      .mean = metric_report_from_series_(report.mean_metrics, metrics),
      .stdev = metric_report_from_series_(report.std_metrics, metrics),
  };
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& frame,
                                                          const DataFrameBatchOptions& data_options,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate(model).labeled_batch_from_dataframe");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(model,x,y)");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(model, x, as_stratify_series(y), cv, train_options, assess_options);
}

namespace workflow {
namespace stage {

#define PLUSS_LYMPH_STAGE_BUILDER(FuncName, KindName, OptionsType)                                 \
  [[nodiscard]] inline TransformSpec FuncName(                                                      \
      OptionsType options = {},                                                                     \
      std::vector<std::int64_t> feature_route = {}) {                                              \
    return TransformSpec{                                                                           \
        .kind = TransformKind::KindName,                                                            \
        .options = options,                                                                         \
        .feature_route = std::move(feature_route),                                                  \
    };                                                                                              \
  }

PLUSS_LYMPH_STAGE_BUILDER(standard_scaler, StandardScaler, prep::StandardScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(minmax_scaler, MinMaxScaler, prep::MinMaxScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(robust_scaler, RobustScaler, prep::RobustScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(simple_imputer, SimpleImputer, prep::SimpleImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(neighbor_imputer, NeighborImputer, prep::NeighborImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(iterative_imputer, IterativeImputer, prep::IterativeImputerOptions)
PLUSS_LYMPH_STAGE_BUILDER(one_hot_encoder, OneHotEncoder, prep::OneHotEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(frequency_encoder, FrequencyEncoder, prep::FrequencyEncoderOptions)
PLUSS_LYMPH_STAGE_BUILDER(vector_normalizer, VectorNormalizer, prep::VectorNormalizerOptions)
PLUSS_LYMPH_STAGE_BUILDER(maxabs_scaler, MaxAbsScaler, prep::MaxAbsScalerOptions)
PLUSS_LYMPH_STAGE_BUILDER(quantile_map, QuantileMap, prep::QuantileMapOptions)
PLUSS_LYMPH_STAGE_BUILDER(power_map, PowerMap, prep::PowerMapOptions)
PLUSS_LYMPH_STAGE_BUILDER(threshold_binarizer, ThresholdBinarizer, prep::ThresholdBinarizerOptions)
PLUSS_LYMPH_STAGE_BUILDER(polynomial_lift, PolynomialLift, prep::PolynomialLiftOptions)

#undef PLUSS_LYMPH_STAGE_BUILDER

} // namespace stage
} // namespace workflow

class Workflow final {
public:
  std::optional<WorkflowModel> model{};
  std::vector<TransformSpec> stages{};

  [[nodiscard]] bool has_model() const noexcept { return model.has_value(); }
  [[nodiscard]] bool trained() const { ensure_model_("trained"); return model->trained(); }
  [[nodiscard]] std::int64_t stage_count() const { ensure_model_("stage_count"); return model->stage_count(); }
  [[nodiscard]] TaskKind task_kind() const { ensure_model_("task_kind"); return model->task_kind(); }
  [[nodiscard]] bool has_model_spec() const { ensure_model_("has_model_spec"); return model->has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const { ensure_model_("has_opaque_transforms"); return model->has_opaque_transforms(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const {
    ensure_model_("model_spec");
    return model->model_spec();
  }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const {
    ensure_model_("transform_specs");
    return model->transform_specs();
  }

  [[nodiscard]] const WorkflowModel& workflow_model() const {
    ensure_model_("workflow_model");
    return *model;
  }

  [[nodiscard]] WorkflowModel& workflow_model() {
    ensure_model_("workflow_model");
    return *model;
  }

  [[nodiscard]] Series coeff() const {
    ensure_model_("coeff");
    return model->coeff;
  }

  [[nodiscard]] Series intercept() const {
    ensure_model_("intercept");
    return model->intercept;
  }

  Workflow& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(batch, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(frame, data_options, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const Series& y,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(x, y, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const pluss::table::Thread& y,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(x, y, options);
    return *this;
  }

  [[nodiscard]] Workflow learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(batch, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& frame,
                               const DataFrameBatchOptions& data_options,
                               const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(frame, data_options, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                              const Series& y,
                              const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(x, y, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                              const pluss::table::Thread& y,
                              const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(x, y, options));
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(batch, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(x, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(frame, data_options, options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(batch, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(x, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(frame, data_options, options);
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(x, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(frame, data_options, options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(batch, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& x,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(x, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(frame, data_options, learn_options, infer_options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(x, y, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(x, y, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(frame, data_options, options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(batch, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(frame, data_options, options);
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    ensure_model_("dump_transform_states");
    return model->dump_transform_states();
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    ensure_model_("dump_model_state");
    return model->dump_model_state();
  }

  Workflow& load_transform_states_(const std::vector<std::vector<Series>>& states) {
    ensure_model_("load_transform_states_");
    model->load_transform_states_(states);
    return *this;
  }

  Workflow& load_model_state_(const std::vector<Series>& state) {
    ensure_model_("load_model_state_");
    model->load_model_state_(state);
    return *this;
  }

  Workflow& mark_trained_(bool trained = true) {
    ensure_model_("mark_trained_");
    model->mark_trained_(trained);
    return *this;
  }

  void save(const std::string& path,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, train_batch, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, x_train, y_train, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, x_train, y_train, train_options, std::move(metadata));
  }

  [[nodiscard]] static Workflow load(const std::string& path) {
    return from_model_(WorkflowModel::load(path));
  }

  [[nodiscard]] static Workflow restore(const io::FlowBundle& bundle) {
    return from_model_(WorkflowModel::restore(bundle));
  }

private:
  void ensure_model_(const char* where) const {
    if (!model.has_value()) {
      throw std::runtime_error(std::string("Workflow::") + where + ": construct as Workflow{ .model = ..., .stages = {...} }.");
    }
    const_cast<Workflow*>(this)->replay_transforms_();
  }

  [[nodiscard]] static Workflow from_model_(WorkflowModel model_value) {
    Workflow out;
    out.model.emplace(std::move(model_value));
    if (out.model.has_value()) {
      out.stages = out.model->transform_specs();
    }
    return out;
  }

  void replay_transforms_() {
    if (!model.has_value() || stages.empty()) return;
    const std::int64_t bound_count = model->stage_count();
    std::size_t begin = 0;
    if (bound_count > 0) {
      begin = static_cast<std::size_t>(std::min<std::int64_t>(bound_count, static_cast<std::int64_t>(stages.size())));
    }
    for (std::size_t i = begin; i < stages.size(); ++i) {
      apply_transform_(*model, stages[i]);
    }
  }

  static void apply_transform_(WorkflowModel& model, const TransformSpec& spec) {
    switch (spec.kind) {
      case TransformKind::StandardScaler:
        model.standard_scaler(std::get<prep::StandardScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::MinMaxScaler:
        model.minmax_scaler(std::get<prep::MinMaxScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::RobustScaler:
        model.robust_scaler(std::get<prep::RobustScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::SimpleImputer:
        model.simple_imputer(std::get<prep::SimpleImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::NeighborImputer:
        model.neighbor_imputer(std::get<prep::NeighborImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::IterativeImputer:
        model.iterative_imputer(std::get<prep::IterativeImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::OneHotEncoder:
        model.one_hot_encoder(std::get<prep::OneHotEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::FrequencyEncoder:
        model.frequency_encoder(std::get<prep::FrequencyEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::VectorNormalizer:
        model.vector_normalizer(std::get<prep::VectorNormalizerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::MaxAbsScaler:
        model.maxabs_scaler(std::get<prep::MaxAbsScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::QuantileMap:
        model.quantile_map(std::get<prep::QuantileMapOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::PowerMap:
        model.power_map(std::get<prep::PowerMapOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::ThresholdBinarizer:
        model.threshold_binarizer(std::get<prep::ThresholdBinarizerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::PolynomialLift:
        model.polynomial_lift(std::get<prep::PolynomialLiftOptions>(spec.options), spec.feature_route);
        return;
    }
  }

};

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& pipeline,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(pipeline.workflow_model(), batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& pipeline,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(pipeline.workflow_model(), x, y, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& pipeline,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(pipeline.workflow_model(), x, y, cv, train_options, assess_options);
}

[[nodiscard]] inline std::vector<std::string> prep_resolve_series_feature_names_(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& pinned_feature_names,
    bool feature_names_pinned,
    const char* where) {
  if (feature_names_pinned) {
    for (const auto& name : pinned_feature_names) {
      if (!frame.has_feature(name)) {
        throw std::runtime_error(std::string(where) + ": missing feature '" + name + "' in input DataFrame.");
      }
    }
    return pinned_feature_names;
  }
  const auto names = frame.series_features().feature_names();
  if (names.empty()) {
    throw std::runtime_error(std::string(where) + ": no Series-backed features found in input DataFrame.");
  }
  return names;
}

[[nodiscard]] inline DataFrameBatchOptions prep_batch_options_for_(const std::vector<std::string>& names) {
  DataFrameBatchOptions options{};
  options.feature_names = names;
  options.layout = ObservationLayout::Rows;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

[[nodiscard]] inline pluss::table::DataFrame prep_merge_mapped_(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& names,
    const Series& mapped,
    const char* where) {
  if (mapped.dim() != 2) {
    throw std::runtime_error(std::string(where) + ": mapped feature matrix must be rank-2.");
  }
  const auto sizes = mapped.sizes();
  if (sizes.size() != 2) {
    throw std::runtime_error(std::string(where) + ": mapped feature matrix must have two dimensions.");
  }
  const std::int64_t sample_count = sizes[0];
  const std::int64_t feature_count = sizes[1];
  if (sample_count != frame.ncases()) {
    throw std::runtime_error(std::string(where) + ": mapped sample count mismatch.");
  }
  if (feature_count != static_cast<std::int64_t>(names.size())) {
    std::vector<std::pair<std::string, pluss::table::Thread>> columns{};
    columns.reserve(static_cast<std::size_t>(feature_count));
    for (std::int64_t j = 0; j < feature_count; ++j) {
      const Series idx = pluss::make_series(
          std::vector<std::int64_t>{j},
          {1},
          Config{}.with_dtype(ScalarType::Int64).with_platform(mapped.platform()));
      const Series col = mapped.index_select(1, idx).reshape({sample_count});
      std::string out_name;
      if (names.size() == 1) {
        out_name = names.front() + "_" + std::to_string(j);
      } else {
        out_name = std::string("feature_") + std::to_string(j);
      }
      columns.emplace_back(std::move(out_name), pluss::table::Thread(col));
    }
    return pluss::table::DataFrame(std::move(columns), frame.cases());
  }

  auto out = frame;
  for (std::int64_t j = 0; j < feature_count; ++j) {
    const Series idx = pluss::make_series(
        std::vector<std::int64_t>{j},
        {1},
        Config{}.with_dtype(ScalarType::Int64).with_platform(mapped.platform()));
    const Series col = mapped.index_select(1, idx).reshape({sample_count});
    out = out.set_features(names[static_cast<std::size_t>(j)], pluss::table::Thread(col));
  }
  return out;
}

inline std::vector<std::string> prep::SimpleImputer::resolved_feature_names_(const pluss::table::DataFrame& frame,
                                                                              const char* where) const {
  return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where);
}

inline DataFrameBatchOptions prep::SimpleImputer::batch_options_for_(const std::vector<std::string>& names) {
  return prep_batch_options_for_(names);
}

inline pluss::table::DataFrame prep::SimpleImputer::merge_mapped_(const pluss::table::DataFrame& frame,
                                                                   const std::vector<std::string>& names,
                                                                   const Series& mapped,
                                                                   const char* where) {
  return prep_merge_mapped_(frame, names, mapped, where);
}

inline void prep::SimpleImputer::refresh_statistics_() {
  statistics_ = Series::undefined();
  if (!learned_) return;
  const auto state = resolve(core_.dump_state(), "prep::SimpleImputer.dump_state");
  if (state.size() >= 2 && state[1].defined()) {
    statistics_ = state[1].reshape({state[1].numel()});
  }
}

inline std::vector<std::string> prep::StandardScaler::resolved_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) const {
  return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where);
}

inline DataFrameBatchOptions prep::StandardScaler::batch_options_for_(const std::vector<std::string>& names) {
  return prep_batch_options_for_(names);
}

inline pluss::table::DataFrame prep::StandardScaler::merge_mapped_(const pluss::table::DataFrame& frame,
                                                                    const std::vector<std::string>& names,
                                                                    const Series& mapped,
                                                                    const char* where) {
  return prep_merge_mapped_(frame, names, mapped, where);
}

inline void prep::StandardScaler::refresh_statistics_() {
  mean_ = Series::undefined();
  scale_ = Series::undefined();
  if (!learned_) return;
  const auto state = resolve(core_.dump_state(), "prep::StandardScaler.dump_state");
  if (state.size() >= 3) {
    if (state[1].defined()) mean_ = state[1].reshape({state[1].numel()});
    if (state[2].defined()) scale_ = state[2].reshape({state[2].numel()});
  }
}

inline prep::SimpleImputer& prep::SimpleImputer::learn(const pluss::table::DataFrame& frame, const TrainOptions& options) {
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.learn");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.learn.feature_batch_from_dataframe");
  resolve(core_.learn(batch, options), "prep::SimpleImputer.learn");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return *this;
}

inline pluss::table::DataFrame prep::SimpleImputer::map(const pluss::table::DataFrame& frame, const InferOptions& options) const {
  if (!learned_) {
    throw std::runtime_error("prep::SimpleImputer.map: call learn() or learn_map() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.map(batch, options), "prep::SimpleImputer.map");
  return merge_mapped_(frame, names, mapped, "prep::SimpleImputer.map");
}

inline pluss::table::DataFrame prep::SimpleImputer::learn_map(const pluss::table::DataFrame& frame,
                                                               const TrainOptions& learn_options,
                                                               const InferOptions& infer_options) {
  const auto names = resolved_feature_names_(frame, "prep::SimpleImputer.learn_map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::SimpleImputer.learn_map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), "prep::SimpleImputer.learn_map");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return merge_mapped_(frame, names, mapped, "prep::SimpleImputer.learn_map");
}

inline prep::StandardScaler& prep::StandardScaler::learn(const pluss::table::DataFrame& frame, const TrainOptions& options) {
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.learn");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.learn.feature_batch_from_dataframe");
  resolve(core_.learn(batch, options), "prep::StandardScaler.learn");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return *this;
}

inline pluss::table::DataFrame prep::StandardScaler::map(const pluss::table::DataFrame& frame, const InferOptions& options) const {
  if (!learned_) {
    throw std::runtime_error("prep::StandardScaler.map: call learn() or learn_map() first.");
  }
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.map(batch, options), "prep::StandardScaler.map");
  return merge_mapped_(frame, names, mapped, "prep::StandardScaler.map");
}

inline pluss::table::DataFrame prep::StandardScaler::learn_map(const pluss::table::DataFrame& frame,
                                                                const TrainOptions& learn_options,
                                                                const InferOptions& infer_options) {
  const auto names = resolved_feature_names_(frame, "prep::StandardScaler.learn_map");
  const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),
                             "prep::StandardScaler.learn_map.feature_batch_from_dataframe");
  const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), "prep::StandardScaler.learn_map");
  feature_names_ = names;
  feature_names_pinned_ = true;
  learned_ = true;
  refresh_statistics_();
  return merge_mapped_(frame, names, mapped, "prep::StandardScaler.learn_map");
}

#define PLUSS_NODEJ_DEFINE_PREP_WRAPPER(WrapperName, Label)                                      \
  inline std::vector<std::string> prep::WrapperName::resolved_feature_names_(                    \
      const pluss::table::DataFrame& frame,                                                       \
      const char* where) const {                                                                  \
    return prep_resolve_series_feature_names_(frame, feature_names_, feature_names_pinned_, where); \
  }                                                                                               \
                                                                                                  \
  inline DataFrameBatchOptions prep::WrapperName::batch_options_for_(const std::vector<std::string>& names) { \
    return prep_batch_options_for_(names);                                                        \
  }                                                                                               \
                                                                                                  \
  inline pluss::table::DataFrame prep::WrapperName::merge_mapped_(                                \
      const pluss::table::DataFrame& frame,                                                       \
      const std::vector<std::string>& names,                                                      \
      const Series& mapped,                                                                       \
      const char* where) {                                                                        \
    return prep_merge_mapped_(frame, names, mapped, where);                                       \
  }                                                                                               \
                                                                                                  \
  inline void prep::WrapperName::refresh_state_() {                                               \
    state_.clear();                                                                               \
    if (!learned_) return;                                                                        \
    try {                                                                                         \
      state_ = resolve(core_.dump_state(), Label ".dump_state");                                  \
    } catch (...) {                                                                               \
      state_.clear();                                                                             \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  inline prep::WrapperName& prep::WrapperName::learn(const pluss::table::DataFrame& frame,       \
                                                      const TrainOptions& options) {              \
    const auto names = resolved_feature_names_(frame, Label ".learn");                           \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".learn.feature_batch_from_dataframe");                      \
    resolve(core_.learn(batch, options), Label ".learn");                                         \
    feature_names_ = names;                                                                       \
    feature_names_pinned_ = true;                                                                 \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return *this;                                                                                 \
  }                                                                                               \
                                                                                                  \
  inline pluss::table::DataFrame prep::WrapperName::map(const pluss::table::DataFrame& frame,    \
                                                         const InferOptions& options) const {     \
    if (!learned_) {                                                                              \
      throw std::runtime_error(std::string(Label) + ".map: call learn() or learn_map() first."); \
    }                                                                                             \
    const auto names = resolved_feature_names_(frame, Label ".map");                             \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".map.feature_batch_from_dataframe");                        \
    const auto mapped = resolve(core_.map(batch, options), Label ".map");                        \
    return merge_mapped_(frame, names, mapped, Label ".map");                                    \
  }                                                                                               \
                                                                                                  \
  inline pluss::table::DataFrame prep::WrapperName::learn_map(const pluss::table::DataFrame& frame, \
                                                               const TrainOptions& learn_options, \
                                                               const InferOptions& infer_options) { \
    const auto names = resolved_feature_names_(frame, Label ".learn_map");                       \
    const auto batch = resolve(feature_batch_from_dataframe(frame, batch_options_for_(names)),   \
                               Label ".learn_map.feature_batch_from_dataframe");                  \
    const auto mapped = resolve(core_.learn_map(batch, learn_options, infer_options), Label ".learn_map"); \
    feature_names_ = names;                                                                       \
    feature_names_pinned_ = true;                                                                 \
    learned_ = true;                                                                              \
    refresh_state_();                                                                             \
    return merge_mapped_(frame, names, mapped, Label ".learn_map");                              \
  }

PLUSS_NODEJ_DEFINE_PREP_WRAPPER(MinMaxScaler, "prep::MinMaxScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(RobustScaler, "prep::RobustScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(NeighborImputer, "prep::NeighborImputer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(IterativeImputer, "prep::IterativeImputer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(OneHotEncoder, "prep::OneHotEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(FrequencyEncoder, "prep::FrequencyEncoder")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(VectorNormalizer, "prep::VectorNormalizer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(MaxAbsScaler, "prep::MaxAbsScaler")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(QuantileMap, "prep::QuantileMap")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(PowerMap, "prep::PowerMap")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(ThresholdBinarizer, "prep::ThresholdBinarizer")
PLUSS_NODEJ_DEFINE_PREP_WRAPPER(PolynomialLift, "prep::PolynomialLift")

#undef PLUSS_NODEJ_DEFINE_PREP_WRAPPER

struct linear_factory {
  [[nodiscard]] static WorkflowModel linear_regression(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::LeastSquares;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel logistic_regression(LinearFamilyOptions options = {}) {
    if (options.objective == LinearObjective::LeastSquares) options.objective = LinearObjective::Logistic;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel hinge_classifier(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::Hinge;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }
};

struct tree_factory {
  [[nodiscard]] static WorkflowModel decision_tree_regressor(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Gini || options.criterion == TreeCriterion::Entropy) {
      options.criterion = TreeCriterion::Mse;
    }
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel decision_tree_classifier(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Mse || options.criterion == TreeCriterion::Mae) {
      options.criterion = TreeCriterion::Gini;
    }
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }
};

struct ensemble_factory {
  [[nodiscard]] static WorkflowModel bagging_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel random_forest_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel bagging_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel random_forest_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel gradient_boost_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel gradient_boost_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(EnsembleFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(EnsembleFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }
};

struct neighbor_factory {
  [[nodiscard]] static WorkflowModel knn_regressor(NeighborFamilyOptions options = {}) {
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel knn_classifier(NeighborFamilyOptions options = {}) {
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(NeighborFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(NeighborFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }
};

struct kernel_factory {
  [[nodiscard]] static WorkflowModel svm_regressor(KernelFamilyOptions options = {}) {
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel svm_classifier(KernelFamilyOptions options = {}) {
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel linear_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel rbf_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel polynomial_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel sigmoid_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel linear_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel rbf_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel polynomial_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel sigmoid_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(KernelFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(KernelFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }
};

struct bayes_factory {
  [[nodiscard]] static WorkflowModel gaussian_nb_classifier(BayesFamilyOptions options = {}) {
    options.use_gaussian = true;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel gaussian_nb_regressor(BayesFamilyOptions options = {}) {
    options.use_gaussian = true;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel regressor(BayesFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(BayesFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }
};

struct cluster_factory {
  [[nodiscard]] static WorkflowModel kmeans(ClusterFamilyOptions options = {}) {
    options.method = ClusterMethod::KMeans;
    return transformer(options);
  }

  [[nodiscard]] static WorkflowModel transformer(ClusterFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Cluster,
        .options = options,
    });
  }
};

struct decompose_factory {
  [[nodiscard]] static WorkflowModel pca(DecomposeFamilyOptions options = {}) {
    options.method = DecomposeMethod::Pca;
    return transformer(options);
  }

  [[nodiscard]] static WorkflowModel transformer(DecomposeFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Decompose,
        .options = options,
    });
  }
};

using OnlineLinearOptions = ::pluss::ai::nodej::online::OnlineLinearOptions;

class OnlineLinearClassifier final {
public:
  explicit OnlineLinearClassifier(OnlineLinearOptions options = {})
      : core_(resolve(::pluss::ai::nodej::online::make_online_linear_classifier(options), "online.make_online_linear_classifier")) {}

  explicit OnlineLinearClassifier(::pluss::ai::nodej::online::OnlineLinearClassifier core)
      : core_(std::move(core)) {}

  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t seen_samples() const noexcept { return core_.seen_samples(); }

  OnlineLinearClassifier& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.learn(batch, options), "online.learn");
    return *this;
  }

  OnlineLinearClassifier& learn_(const pluss::table::DataFrame& x,
                                 const Series& y,
                                 const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  OnlineLinearClassifier& learn_(const pluss::table::DataFrame& x,
                                 const pluss::table::Thread& y,
                                 const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  OnlineLinearClassifier& advance_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.advance(batch, options), "online.advance");
    return *this;
  }

  OnlineLinearClassifier& advance_(const pluss::table::DataFrame& x,
                                   const Series& y,
                                   const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.advance_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.advance_(x,y)");
    feature_names_ = names;
    return advance_(batch, options);
  }

  OnlineLinearClassifier& advance_(const pluss::table::DataFrame& x,
                                   const pluss::table::Thread& y,
                                   const TrainOptions& options = {}) {
    return advance_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer_scores(batch, options), "online.infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x,
                                    const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer_scores(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer_scores(x)");
    return infer_scores(batch, options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer(batch, options), "online.infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer(x)");
    return infer(batch, options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(core_.assess(batch, options), "online.assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.assess_series(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.assess_series(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(batch, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Classification, options));
  }

  [[nodiscard]] const ::pluss::ai::nodej::online::OnlineLinearClassifier& core() const noexcept { return core_; }
  [[nodiscard]] ::pluss::ai::nodej::online::OnlineLinearClassifier& core() noexcept { return core_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(const std::vector<std::string>& feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = feature_names;
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.series_features().feature_names();
    if (names.empty()) {
      throw std::runtime_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static FeatureBatch feature_from_x_(const pluss::table::DataFrame& x,
                                                    const std::vector<std::string>& names,
                                                    const char* where) {
    return resolve(feature_batch_from_dataframe(x, default_data_options_(names)), std::string(where) + ".feature_batch_from_dataframe");
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     const std::vector<std::string>& names,
                                                     const char* where) {
    const FeatureBatch x_batch = feature_from_x_(x, names, where);
    LabeledBatch batch{
        .features = x_batch,
        .y = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()}),
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  ::pluss::ai::nodej::online::OnlineLinearClassifier core_{};
  std::vector<std::string> feature_names_{};
};

class OnlineLinearRegressor final {
public:
  explicit OnlineLinearRegressor(OnlineLinearOptions options = {})
      : core_(resolve(::pluss::ai::nodej::online::make_online_linear_regressor(options), "online.make_online_linear_regressor")) {}

  explicit OnlineLinearRegressor(::pluss::ai::nodej::online::OnlineLinearRegressor core)
      : core_(std::move(core)) {}

  [[nodiscard]] bool trained() const noexcept { return core_.trained(); }
  [[nodiscard]] std::int64_t seen_samples() const noexcept { return core_.seen_samples(); }

  OnlineLinearRegressor& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.learn(batch, options), "online.learn");
    return *this;
  }

  OnlineLinearRegressor& learn_(const pluss::table::DataFrame& x,
                                const Series& y,
                                const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  OnlineLinearRegressor& learn_(const pluss::table::DataFrame& x,
                                const pluss::table::Thread& y,
                                const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  OnlineLinearRegressor& advance_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(core_.advance(batch, options), "online.advance");
    return *this;
  }

  OnlineLinearRegressor& advance_(const pluss::table::DataFrame& x,
                                  const Series& y,
                                  const TrainOptions& options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.advance_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.advance_(x,y)");
    feature_names_ = names;
    return advance_(batch, options);
  }

  OnlineLinearRegressor& advance_(const pluss::table::DataFrame& x,
                                  const pluss::table::Thread& y,
                                  const TrainOptions& options = {}) {
    return advance_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(core_.infer(batch, options), "online.infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.infer(x)");
    const FeatureBatch batch = feature_from_x_(x, names, "online.infer(x)");
    return infer(batch, options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(core_.assess(batch, options), "online.assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "online.assess_series(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "online.assess_series(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(batch, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return metric_report_from_series_(assess_series(x, y, options), default_assess_metrics_(TaskKind::Regression, options));
  }

  [[nodiscard]] const ::pluss::ai::nodej::online::OnlineLinearRegressor& core() const noexcept { return core_; }
  [[nodiscard]] ::pluss::ai::nodej::online::OnlineLinearRegressor& core() noexcept { return core_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(const std::vector<std::string>& feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = feature_names;
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.series_features().feature_names();
    if (names.empty()) {
      throw std::runtime_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static FeatureBatch feature_from_x_(const pluss::table::DataFrame& x,
                                                    const std::vector<std::string>& names,
                                                    const char* where) {
    return resolve(feature_batch_from_dataframe(x, default_data_options_(names)), std::string(where) + ".feature_batch_from_dataframe");
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     const std::vector<std::string>& names,
                                                     const char* where) {
    const FeatureBatch x_batch = feature_from_x_(x, names, where);
    LabeledBatch batch{
        .features = x_batch,
        .y = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()}),
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  ::pluss::ai::nodej::online::OnlineLinearRegressor core_{};
  std::vector<std::string> feature_names_{};
};

struct online_factory {
  [[nodiscard]] static OnlineLinearClassifier linear_classifier(OnlineLinearOptions options = {}) {
    return OnlineLinearClassifier(options);
  }

  [[nodiscard]] static OnlineLinearRegressor linear_regressor(OnlineLinearOptions options = {}) {
    return OnlineLinearRegressor(options);
  }
};

namespace selection {

struct Candidate {
  std::string name{};
  Workflow workflow{};

  Candidate(std::string in_name, Workflow in_workflow)
      : name(std::move(in_name)), workflow(std::move(in_workflow)) {}
  Candidate(Candidate&&) noexcept = default;
  Candidate& operator=(Candidate&&) noexcept = default;
  Candidate(const Candidate&) = delete;
  Candidate& operator=(const Candidate&) = delete;
};

struct Trial {
  std::string name{};
  double objective = std::numeric_limits<double>::quiet_NaN();
  CrossValidationReport cv{};
};

struct TuneReport {
  std::vector<Trial> trials{};
  std::int64_t best_index = -1;
  std::string best_name{};
  MetricKind objective_metric = MetricKind::Accuracy;
  TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto;
};

struct TuneResult {
  TuneReport report{};
  Workflow best_workflow{};
};

struct SpecTuneResult {
  ::pluss::ai::nodej::TuneReport report{};
  ModelSpec best_spec{};
};

[[nodiscard]] inline bool minimize_metric_(MetricKind metric, TuneObjectivePolicy policy) {
  if (policy == TuneObjectivePolicy::Minimize) return true;
  if (policy == TuneObjectivePolicy::Maximize) return false;
  switch (metric) {
    case MetricKind::LogLoss:
    case MetricKind::Mse:
    case MetricKind::Rmse:
    case MetricKind::Mae:
    case MetricKind::QuantileLoss:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] inline double scalar_metric_(const MetricReport& report, MetricKind metric) {
  const std::size_t n = std::min(report.requested.size(), static_cast<std::size_t>(report.values.numel()));
  const auto values = report.values.to(pluss::pCPU, ScalarType::Float64).reshape({report.values.numel()}).to_vec<double>();
  for (std::size_t i = 0; i < n && i < values.size(); ++i) {
    if (report.requested[i] == metric) return values[i];
  }
  throw std::runtime_error("selection::tune: objective metric not present in assessment metrics.");
}

[[nodiscard]] inline TuneResult tune(std::vector<Candidate> candidates,
                                     const pluss::table::DataFrame& x,
                                     const Series& y,
                                     CrossValidationOptions cv = {},
                                     TrainOptions train_options = {},
                                     AssessOptions assess_options = {},
                                     MetricKind objective_metric = MetricKind::Accuracy,
                                     TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  if (candidates.empty()) {
    throw std::runtime_error("selection::tune: at least one candidate is required.");
  }
  if (assess_options.metrics.empty()) {
    assess_options.metrics = {objective_metric};
  } else if (std::find(assess_options.metrics.begin(), assess_options.metrics.end(), objective_metric) == assess_options.metrics.end()) {
    assess_options.metrics.push_back(objective_metric);
  }

  const bool minimize = minimize_metric_(objective_metric, objective_policy);
  TuneResult result{};
  result.report.objective_metric = objective_metric;
  result.report.objective_policy = objective_policy;
  result.report.trials.reserve(candidates.size());

  std::int64_t best_index = -1;
  double best_objective = std::numeric_limits<double>::quiet_NaN();

  for (std::size_t i = 0; i < candidates.size(); ++i) {
    auto cv_report = cross_validate(candidates[i].workflow, x, y, cv, train_options, assess_options);
    const double objective = scalar_metric_(cv_report.mean, objective_metric);
    result.report.trials.push_back(Trial{
        .name = candidates[i].name,
        .objective = objective,
        .cv = std::move(cv_report),
    });

    if (best_index < 0) {
      best_index = static_cast<std::int64_t>(i);
      best_objective = objective;
      continue;
    }
    const bool better = minimize ? (objective < best_objective) : (objective > best_objective);
    if (better) {
      best_index = static_cast<std::int64_t>(i);
      best_objective = objective;
    }
  }

  result.report.best_index = best_index;
  result.report.best_name = candidates[static_cast<std::size_t>(best_index)].name;
  result.best_workflow = std::move(candidates[static_cast<std::size_t>(best_index)].workflow);
  return result;
}

[[nodiscard]] inline TuneResult tune(std::vector<Candidate> candidates,
                                     const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     CrossValidationOptions cv = {},
                                     TrainOptions train_options = {},
                                     AssessOptions assess_options = {},
                                     MetricKind objective_metric = MetricKind::Accuracy,
                                     TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return tune(std::move(candidates),
              x,
              as_stratify_series(y),
              cv,
              train_options,
              assess_options,
              objective_metric,
              objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<ModelSpec>& candidates,
                                               const LabeledBatch& batch,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  auto report = resolve(::pluss::ai::nodej::sweep(candidates,
                                                  batch,
                                                  split,
                                                  train_options,
                                                  assess_options,
                                                  objective_metric,
                                                  objective_policy),
                        "selection::tune_specs.sweep");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    throw std::runtime_error("selection::tune_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<ModelSpec>& candidates,
                                               const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  const auto names = cross_validate_default_feature_names_(x, "selection::tune_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::tune_specs(x,y)");
  return tune_specs(candidates, batch, split, train_options, assess_options, objective_metric, objective_policy);
}

[[nodiscard]] inline SpecTuneResult tune_specs(const std::vector<ModelSpec>& candidates,
                                               const pluss::table::DataFrame& x,
                                               const pluss::table::Thread& y,
                                               const SplitOptions& split = {},
                                               const TrainOptions& train_options = {},
                                               const AssessOptions& assess_options = {},
                                               MetricKind objective_metric = MetricKind::Accuracy,
                                               TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto) {
  return tune_specs(candidates,
                    x,
                    as_stratify_series(y),
                    split,
                    train_options,
                    assess_options,
                    objective_metric,
                    objective_policy);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const LabeledBatch& batch,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  auto report = resolve(::pluss::ai::nodej::probe(sampler,
                                                  trials,
                                                  batch,
                                                  split,
                                                  train_options,
                                                  assess_options,
                                                  objective_metric,
                                                  objective_policy,
                                                  random),
                        "selection::probe_specs.probe");
  if (report.best_index < 0 || static_cast<std::size_t>(report.best_index) >= report.trials.size()) {
    throw std::runtime_error("selection::probe_specs: best_index is out of range.");
  }
  return SpecTuneResult{
      .report = report,
      .best_spec = report.trials[static_cast<std::size_t>(report.best_index)].spec,
  };
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const pluss::table::DataFrame& x,
                                                const Series& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  const auto names = cross_validate_default_feature_names_(x, "selection::probe_specs(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "selection::probe_specs(x,y)");
  return probe_specs(sampler,
                     trials,
                     batch,
                     split,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy,
                     random);
}

[[nodiscard]] inline SpecTuneResult probe_specs(const ProbeSpecSampler& sampler,
                                                std::int64_t trials,
                                                const pluss::table::DataFrame& x,
                                                const pluss::table::Thread& y,
                                                const SplitOptions& split = {},
                                                const TrainOptions& train_options = {},
                                                const AssessOptions& assess_options = {},
                                                MetricKind objective_metric = MetricKind::Accuracy,
                                                TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto,
                                                RandomState random = {}) {
  return probe_specs(sampler,
                     trials,
                     x,
                     as_stratify_series(y),
                     split,
                     train_options,
                     assess_options,
                     objective_metric,
                     objective_policy,
                     random);
}

} // namespace selection

} // namespace pluss::ai::lymph
