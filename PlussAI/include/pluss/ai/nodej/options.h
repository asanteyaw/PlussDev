#pragma once

#include "pluss/ai/nodej/types.h"

#include <cstdint>
#include <vector>

namespace pluss::ai::nodej {

enum class TaskKind : std::uint8_t {
  Classification = 0,
  Regression = 1,
  Transform = 2,
};

enum class ObservationLayout : std::uint8_t {
  Rows = 0,
  Columns = 1,
};

enum class MissingPolicy : std::uint8_t {
  Reject = 0,
  Drop = 1,
  ImputeMean = 2,
  ImputeMedian = 3,
  KeepNaN = 4,
};

enum class UnknownCategoryPolicy : std::uint8_t {
  Error = 0,
  MissingBucket = 1,
  Ignore = 2,
};

enum class MetricKind : std::uint8_t {
  Accuracy = 0,
  BalancedAccuracy = 1,
  Precision = 2,
  Recall = 3,
  F1 = 4,
  RocAuc = 5,
  LogLoss = 6,
  Mse = 7,
  Rmse = 8,
  Mae = 9,
  R2 = 10,
  QuantileLoss = 11,
  Sensitivity = 12,
  Specificity = 13,
  GeometricMean = 14,
  BalanceQuality = 15,
  Mcc = 16,               // Matthews correlation coefficient (from confusion matrix)
  CohenKappa = 17,        // unweighted Cohen's kappa (from confusion matrix)
  Gini = 18,              // 2 * RocAuc - 1 (needs scores)
  AveragePrecision = 19,  // area under the precision-recall curve (needs scores)
  Brier = 20,             // mean squared error of predicted probabilities (loss; needs probabilities)
};

// Exhaustive (no default) so a new MetricKind fails to compile here until its
// direction is decided -- the single shared answer to "does a larger value
// mean better?" Several call sites (tune.cpp, selection::minimize_metric_,
// sparse_runtime.cpp) currently carry their own private copy of this same
// switch; new code should call this one instead of adding a fourth.
[[nodiscard]] inline bool metric_prefers_max(MetricKind metric) {
  switch (metric) {
    case MetricKind::Accuracy:
    case MetricKind::BalancedAccuracy:
    case MetricKind::Precision:
    case MetricKind::Recall:
    case MetricKind::Sensitivity:
    case MetricKind::F1:
    case MetricKind::RocAuc:
    case MetricKind::Specificity:
    case MetricKind::GeometricMean:
    case MetricKind::BalanceQuality:
    case MetricKind::R2:
    case MetricKind::Mcc:
    case MetricKind::CohenKappa:
    case MetricKind::Gini:
    case MetricKind::AveragePrecision:
      return true;
    case MetricKind::LogLoss:
    case MetricKind::Mse:
    case MetricKind::Rmse:
    case MetricKind::Mae:
    case MetricKind::QuantileLoss:
    case MetricKind::Brier:
      return false;
  }
  return true;
}

enum class CVPlanKind : std::uint8_t {
  Holdout = 0,
  CrossVal = 1,
  StratifiedCrossVal = 2,
  GroupCrossVal = 3,
  TSCrossVal = 4,
};

enum class SearchKind : std::uint8_t {
  None = 0,
  Grid = 1,
  Random = 2,
  Bayesian = 3,
  SuccessiveHalving = 4,
};

enum class ClassWeightMode : std::uint8_t {
  None = 0,
  Balanced = 1,
  Custom = 2,
};

enum class ScoreCalibration : std::uint8_t {
  None = 0,
  Platt = 1,
  Isotonic = 2,
  Temperature = 3,
};

enum class BalanceTargetKind : std::uint8_t {
  Keep = 0,
  Equalize = 1,
  Ratio = 2,
  Counts = 3,
};

enum class BalanceGrowthKind : std::uint8_t {
  ReuseRows = 0,
  BlendNeighbors = 1,
};

enum class BalanceCleanKind : std::uint8_t {
  None = 0,
  BoundaryPairs = 1,
  NeighborAgreement = 2,
  BoundaryPairsAndNeighborAgreement = 3,
};

enum class BalanceCleanVote : std::uint8_t {
  AnyDisagreement = 0,
  MajorityDisagreement = 1,
};

struct BalanceTarget {
  BalanceTargetKind kind = BalanceTargetKind::Keep;
  double ratio = 1.0;
  Series counts = pluss::series::undefined();
  BalanceGrowthKind growth = BalanceGrowthKind::ReuseRows;
  std::int64_t neighbors = 5;
  BalanceCleanKind clean = BalanceCleanKind::None;
  std::int64_t clean_neighbors = 3;
  BalanceCleanVote clean_vote = BalanceCleanVote::AnyDisagreement;
  bool protect_small_classes = true;
};

struct RandomState {
  std::uint64_t seed = 0;
  bool deterministic = true;
};

struct TrainOptions {
  ObservationLayout layout = ObservationLayout::Rows;
  MissingPolicy missing = MissingPolicy::Reject;
  UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error;
  std::int64_t max_iterations = 1000;
  double tolerance = 1e-6;
  bool warm_start = false;
  bool track_diagnostics = true;
  RandomState random{};
  Series sample_weights = pluss::series::undefined();
  ClassWeightMode class_weight = ClassWeightMode::None;
  // For ClassWeightMode::Custom use [negative_class_weight, positive_class_weight]
  // for binary classifiers. Multiclass OvR applies this pair per one-vs-rest leg.
  Series class_weights = pluss::series::undefined();
  ScoreCalibration calibration = ScoreCalibration::None;
  std::int64_t calibration_iterations = 400;
  double calibration_tolerance = 1e-6;
  // Optional coefficient bounds used by minimax-constrained updates in
  // gradient-based learners (currently linear classification).
  Series parameter_lower_bounds = pluss::series::undefined();
  Series parameter_upper_bounds = pluss::series::undefined();
  // Classification row balancing/cleaning applied before estimator fitting.
  // Default Keep + no cleaning leaves training data unchanged.
  BalanceTarget balance{};
};

struct InferOptions {
  ObservationLayout layout = ObservationLayout::Rows;
  MissingPolicy missing = MissingPolicy::Reject;
  UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error;
  // Binary decision threshold in probability space for classification.
  double positive_threshold = 0.5;
};

struct AssessOptions {
  ObservationLayout layout = ObservationLayout::Rows;
  std::vector<MetricKind> metrics{};
  Series sample_weights = pluss::series::undefined();
  // Binary decision threshold in probability space for classification metrics
  // that require hard labels (for example Accuracy/F1).
  double positive_threshold = 0.5;
  // Quantile used when MetricKind::QuantileLoss is requested (0 < q < 1).
  double regression_quantile = 0.5;
};

struct SplitOptions {
  CVPlanKind plan = CVPlanKind::Holdout;
  double holdout_fraction = 0.2;
  std::int64_t folds = 5;
  // Repetition count used when plan == CVPlanKind::StratifiedCrossVal.
  std::int64_t repeats = 1;
  // Time-series CV controls (used when plan == CVPlanKind::TSCrossVal).
  // If time_test_size <= 0, test window is inferred from the sample/fold layout.
  // If time_max_train_size <= 0, train window is unbounded (expanding).
  std::int64_t time_gap = 0;
  std::int64_t time_test_size = 0;
  std::int64_t time_max_train_size = 0;
  bool stratified = false;
  // Optional per-sample group labels used when plan == CVPlanKind::GroupCrossVal.
  // Length must match the sample count.
  Series groups = pluss::series::undefined();
  bool reliability_defaults = true;
  bool shuffle = true;
  // Parallel workers for fold evaluation in cross_assess.
  // <= 0 uses hardware-concurrency auto sizing.
  std::int64_t max_parallel = 0;
  RandomState random{};
};

struct TuneOptions {
  SearchKind search = SearchKind::None;
  std::int64_t trials = 0;
  std::int64_t max_parallel = 1;
  SplitOptions split{};
  RandomState random{};
};

} // namespace pluss::ai::nodej
