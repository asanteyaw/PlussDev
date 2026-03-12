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
};

enum class CVPlanKind : std::uint8_t {
  Holdout = 0,
  KFold = 1,
  StratifiedKFold = 2,
  GroupKFold = 3,
  TimeSeries = 4,
};

enum class SearchKind : std::uint8_t {
  None = 0,
  Grid = 1,
  Random = 2,
  Bayesian = 3,
  SuccessiveHalving = 4,
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
  Series sample_weights = Series::undefined();
  // Optional coefficient bounds used by minimax-constrained updates in
  // gradient-based learners (currently linear classification).
  Series parameter_lower_bounds = Series::undefined();
  Series parameter_upper_bounds = Series::undefined();
};

struct InferOptions {
  ObservationLayout layout = ObservationLayout::Rows;
  MissingPolicy missing = MissingPolicy::Reject;
  UnknownCategoryPolicy unknown_categorical = UnknownCategoryPolicy::Error;
};

struct AssessOptions {
  ObservationLayout layout = ObservationLayout::Rows;
  std::vector<MetricKind> metrics{};
  Series sample_weights = Series::undefined();
  // Quantile used when MetricKind::QuantileLoss is requested (0 < q < 1).
  double regression_quantile = 0.5;
};

struct SplitOptions {
  CVPlanKind plan = CVPlanKind::Holdout;
  double holdout_fraction = 0.2;
  std::int64_t folds = 5;
  bool stratified = false;
  bool shuffle = true;
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
