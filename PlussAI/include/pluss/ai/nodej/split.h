#pragma once

#include "pluss/ai/nodej/dataset.h"
#include "pluss/ai/nodej/options.h"
#include "pluss/ai/nodej/status.h"

#include <vector>

namespace pluss::ai::nodej {

struct IndexSplit {
  Series train_indices = Series::undefined();
  Series test_indices = Series::undefined();
};

struct SplitPlan {
  std::vector<IndexSplit> folds{};
};

StatusOr<IndexSplit> holdout_split(std::int64_t sample_count, const SplitOptions& options = {});
StatusOr<IndexSplit> strat_holdout_split(const Series& labels, const SplitOptions& options = {});

StatusOr<SplitPlan> fold_plan(std::int64_t sample_count, const SplitOptions& options = {});
StatusOr<SplitPlan> strat_fold_plan(const Series& labels, const SplitOptions& options = {});

StatusOr<FeatureBatch> slice_feature_batch(const FeatureBatch& in, const Series& indices);
StatusOr<LabeledBatch> slice_labeled_batch(const LabeledBatch& in, const Series& indices);

} // namespace pluss::ai::nodej
