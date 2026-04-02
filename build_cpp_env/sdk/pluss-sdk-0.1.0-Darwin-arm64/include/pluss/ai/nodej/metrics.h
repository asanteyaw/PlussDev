#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/split.h"

#include <vector>

namespace pluss::ai::nodej {

struct FoldAssessResult {
  Series metrics = Series::undefined();
  Series train_indices = Series::undefined();
  Series test_indices = Series::undefined();
};

struct CrossAssessReport {
  std::vector<FoldAssessResult> folds{};
  std::vector<MetricKind> metrics{};
  Series mean_metrics = Series::undefined();
  Series std_metrics = Series::undefined();
};

StatusOr<CrossAssessReport> cross_assess(const ModelSpec& spec,
                                         const LabeledBatch& batch,
                                         const SplitOptions& split = {},
                                         const TrainOptions& train_options = {},
                                         const AssessOptions& assess_options = {});

} // namespace pluss::ai::nodej
