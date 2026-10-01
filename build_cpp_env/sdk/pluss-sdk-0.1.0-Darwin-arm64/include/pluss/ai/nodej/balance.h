#pragma once

#include "pluss/ai/nodej/dataset.h"

namespace pluss::ai::nodej {

struct ClassMix {
  Series labels = pluss::series::undefined();
  Series counts = pluss::series::undefined();
  Series proportions = pluss::series::undefined();
  Series inverse_frequency = pluss::series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t class_count = 0;
  double imbalance_ratio = 1.0;
};

struct WeightPlan {
  Series weights = pluss::series::undefined();
  Series class_weights = pluss::series::undefined();
  Series sample_weights = pluss::series::undefined();
  ClassMix mix{};
  double mean_weight = 1.0;
};

struct BalancePlan {
  Series indices = pluss::series::undefined();
  Series target_counts = pluss::series::undefined();
  Series synthetic_base_indices = pluss::series::undefined();
  Series synthetic_neighbor_indices = pluss::series::undefined();
  Series synthetic_steps = pluss::series::undefined();
  Series synthetic_labels = pluss::series::undefined();
  Series removed_indices = pluss::series::undefined();
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
