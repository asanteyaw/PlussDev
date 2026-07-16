#pragma once

#include "pluss/ai/nodej/dataset.h"

namespace pluss::ai::nodej {

struct ClassMix {
  Series labels = Series::undefined();
  Series counts = Series::undefined();
  Series proportions = Series::undefined();
  Series inverse_frequency = Series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t class_count = 0;
  double imbalance_ratio = 1.0;
};

struct WeightPlan {
  Series weights = Series::undefined();
  Series class_weights = Series::undefined();
  Series sample_weights = Series::undefined();
  ClassMix mix{};
  double mean_weight = 1.0;
};

struct BalancePlan {
  Series indices = Series::undefined();
  Series target_counts = Series::undefined();
  Series synthetic_base_indices = Series::undefined();
  Series synthetic_neighbor_indices = Series::undefined();
  Series synthetic_steps = Series::undefined();
  Series synthetic_labels = Series::undefined();
  Series removed_indices = Series::undefined();
  ClassMix before{};
  ClassMix after{};
  bool uses_replacement = false;
  bool uses_synthesis = false;
  bool uses_cleaning = false;
};

[[nodiscard]] StatusOr<ClassMix> class_mix(const Series& labels);

[[nodiscard]] StatusOr<WeightPlan> resolve_weight_plan(const Series& labels, const Series& batch_sample_weights, const TrainOptions& options);

[[nodiscard]] StatusOr<Series> resolve_training_weights(const Series& labels, const Series& batch_sample_weights, const TrainOptions& options);

[[nodiscard]] StatusOr<BalancePlan> plan_balance(const Series& labels, const BalanceTarget& target, RandomState random = {});

[[nodiscard]] StatusOr<BalancePlan> plan_balance(const LabeledBatch& batch, const BalanceTarget& target, RandomState random = {});

[[nodiscard]] StatusOr<LabeledBatch> balance_rows(const LabeledBatch& batch, const BalanceTarget& target, RandomState random = {});

} // namespace pluss::ai::nodej
