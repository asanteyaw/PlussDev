#include "pluss/ai/nodej/metrics.h"

#include <algorithm>
#include <future>
#include <thread>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] std::vector<MetricKind> default_metrics(TaskKind task) {
  if (task == TaskKind::Classification) return {MetricKind::Accuracy};
  if (task == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  return {};
}

[[nodiscard]] StatusOr<SplitPlan> make_cross_plan(const ModelSpec& spec,
                                                  const LabeledBatch& batch,
                                                  const SplitOptions& split) {
  if (split.plan == CVPlanKind::Holdout) {
    StatusOr<IndexSplit> holdout_or =
        (split.stratified && spec.task == TaskKind::Classification)
            ? strat_holdout_split(batch.y, split)
            : holdout_split(batch.features.sample_count(), split);
    if (!holdout_or.ok()) return StatusOr<SplitPlan>(holdout_or.status());
    SplitPlan plan{};
    plan.folds.push_back(holdout_or.move_value());
    return StatusOr<SplitPlan>(std::move(plan));
  }
  if (split.plan == CVPlanKind::KFold) {
    if (split.stratified && spec.task == TaskKind::Classification) {
      return strat_fold_plan(batch.y, split);
    }
    return fold_plan(batch.features.sample_count(), split);
  }
  if (split.plan == CVPlanKind::StratifiedKFold) {
    if (spec.task != TaskKind::Classification) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("cross_assess StratifiedKFold requires TaskKind::Classification."));
    }
    return strat_fold_plan(batch.y, split);
  }
  if (split.plan == CVPlanKind::TimeSeries) {
    return time_series_fold_plan(batch.features.sample_count(), split);
  }
  if (split.plan == CVPlanKind::GroupKFold) {
    if (!split.groups.defined()) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("cross_assess GroupKFold requires SplitOptions.groups to be defined."));
    }
    if (split.groups.numel() != batch.features.sample_count()) {
      return StatusOr<SplitPlan>(
          Status::shape_mismatch("cross_assess GroupKFold requires SplitOptions.groups length to match sample count."));
    }
    return group_fold_plan(split.groups, split);
  }
  return StatusOr<SplitPlan>(Status::invalid_argument("cross_assess received unknown SplitOptions.plan."));
}

[[nodiscard]] SplitOptions resolved_split_defaults(const ModelSpec& spec, SplitOptions split) {
  if (!split.reliability_defaults) return split;
  if (spec.task != TaskKind::Classification) return split;
  if (!split.stratified
      && (split.plan == CVPlanKind::Holdout || split.plan == CVPlanKind::KFold)) {
    split.stratified = true;
  }
  return split;
}

[[nodiscard]] Status with_fold_context(Status status,
                                       std::int64_t fold_index,
                                       std::int64_t fold_count,
                                       std::int64_t train_samples,
                                       std::int64_t test_samples) {
  if (status.ok()) return status;
  const std::string prefix = "cross_assess fold "
                             + std::to_string(fold_index + 1) + "/" + std::to_string(fold_count)
                             + " (train=" + std::to_string(train_samples)
                             + ", test=" + std::to_string(test_samples) + "): ";
  status.message = prefix + status.message;
  return status;
}

template <class ModelT>
[[nodiscard]] StatusOr<FoldAssessResult> learn_and_assess_fold(ModelT& model,
                                                               const LabeledBatch& train,
                                                               const LabeledBatch& test,
                                                               const TrainOptions& train_options,
                                                               const AssessOptions& assess_options,
                                                               const IndexSplit& split) {
  Status learn_status = model.learn(train, train_options);
  if (!learn_status.ok()) return StatusOr<FoldAssessResult>(std::move(learn_status));
  StatusOr<Series> fold_metrics_or = model.assess(test, assess_options);
  if (!fold_metrics_or.ok()) return StatusOr<FoldAssessResult>(fold_metrics_or.status());
  Series fold_metrics = fold_metrics_or.move_value().to(ScalarType::Float64);
  FoldAssessResult fold{};
  fold.metrics = fold_metrics.reshape({fold_metrics.numel()});
  fold.train_indices = split.train_indices;
  fold.test_indices = split.test_indices;
  return StatusOr<FoldAssessResult>(std::move(fold));
}

[[nodiscard]] std::int64_t resolved_cv_parallelism(std::size_t fold_count, const SplitOptions& split) {
  if (fold_count <= 1) return 1;
  if (split.max_parallel > 0) {
    return std::max<std::int64_t>(
        1,
        std::min<std::int64_t>(static_cast<std::int64_t>(fold_count), split.max_parallel));
  }
  const std::uint32_t hw = std::thread::hardware_concurrency();
  const std::int64_t cap = hw == 0 ? 1 : static_cast<std::int64_t>(hw);
  return std::max<std::int64_t>(1, std::min<std::int64_t>(static_cast<std::int64_t>(fold_count), cap));
}

} // namespace

StatusOr<CrossAssessReport> cross_assess(const ModelSpec& spec,
                                         const LabeledBatch& batch,
                                         const SplitOptions& split,
                                         const TrainOptions& train_options,
                                         const AssessOptions& assess_options) {
  if (spec.task != TaskKind::Classification && spec.task != TaskKind::Regression) {
    return StatusOr<CrossAssessReport>(
        Status::invalid_argument("cross_assess currently supports TaskKind::Classification or TaskKind::Regression."));
  }

  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<CrossAssessReport>(std::move(data_status));

  const SplitOptions effective_split = resolved_split_defaults(spec, split);
  StatusOr<SplitPlan> plan_or = make_cross_plan(spec, batch, effective_split);
  if (!plan_or.ok()) return StatusOr<CrossAssessReport>(plan_or.status());
  SplitPlan plan = plan_or.move_value();
  if (plan.folds.empty()) {
    return StatusOr<CrossAssessReport>(Status::internal("cross_assess produced an empty fold plan."));
  }

  CrossAssessReport report{};
  report.metrics = assess_options.metrics.empty() ? default_metrics(spec.task) : assess_options.metrics;
  if (report.metrics.empty()) {
    return StatusOr<CrossAssessReport>(Status::invalid_argument("cross_assess requires at least one assessment metric."));
  }
  AssessOptions fold_assess_options = assess_options;
  fold_assess_options.metrics = report.metrics;
  report.folds.reserve(plan.folds.size());

  struct PreparedFold {
    IndexSplit split{};
    LabeledBatch train{};
    LabeledBatch test{};
  };
  std::vector<PreparedFold> prepared{};
  prepared.reserve(plan.folds.size());

  for (std::size_t fi = 0; fi < plan.folds.size(); ++fi) {
    const auto& fold = plan.folds[fi];
    StatusOr<LabeledBatch> train_or = slice_labeled_batch(batch, fold.train_indices);
    if (!train_or.ok()) {
      return StatusOr<CrossAssessReport>(with_fold_context(train_or.status(),
                                                           static_cast<std::int64_t>(fi),
                                                           static_cast<std::int64_t>(plan.folds.size()),
                                                           fold.train_indices.numel(),
                                                           fold.test_indices.numel()));
    }
    StatusOr<LabeledBatch> test_or = slice_labeled_batch(batch, fold.test_indices);
    if (!test_or.ok()) {
      return StatusOr<CrossAssessReport>(with_fold_context(test_or.status(),
                                                           static_cast<std::int64_t>(fi),
                                                           static_cast<std::int64_t>(plan.folds.size()),
                                                           fold.train_indices.numel(),
                                                           fold.test_indices.numel()));
    }
    prepared.push_back(PreparedFold{
        .split = fold,
        .train = train_or.move_value(),
        .test = test_or.move_value(),
    });
  }

  std::vector<StatusOr<FoldAssessResult>> fold_results{};
  fold_results.reserve(prepared.size());
  auto eval_fold = [&](std::size_t fi) -> StatusOr<FoldAssessResult> {
    const PreparedFold& pf = prepared[fi];
    if (spec.task == TaskKind::Classification) {
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
      if (!model_or.ok()) return StatusOr<FoldAssessResult>(model_or.status());
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      return learn_and_assess_fold(*model, pf.train, pf.test, train_options, fold_assess_options, pf.split);
    }
    StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
    if (!model_or.ok()) return StatusOr<FoldAssessResult>(model_or.status());
    std::unique_ptr<RegressorBase> model = model_or.move_value();
    return learn_and_assess_fold(*model, pf.train, pf.test, train_options, fold_assess_options, pf.split);
  };

  const std::int64_t workers = resolved_cv_parallelism(prepared.size(), effective_split);
  if (workers <= 1) {
    for (std::size_t fi = 0; fi < prepared.size(); ++fi) {
      fold_results.push_back(eval_fold(fi));
    }
  } else {
    for (std::size_t start = 0; start < prepared.size(); start += static_cast<std::size_t>(workers)) {
      const std::size_t end = std::min(prepared.size(), start + static_cast<std::size_t>(workers));
      std::vector<std::future<StatusOr<FoldAssessResult>>> futures{};
      futures.reserve(end - start);
      for (std::size_t fi = start; fi < end; ++fi) {
        futures.emplace_back(std::async(std::launch::async, [&, fi]() { return eval_fold(fi); }));
      }
      for (auto& future : futures) {
        fold_results.push_back(future.get());
      }
    }
  }

  for (std::size_t fi = 0; fi < fold_results.size(); ++fi) {
    const PreparedFold& pf = prepared[fi];
    if (!fold_results[fi].ok()) {
      return StatusOr<CrossAssessReport>(with_fold_context(fold_results[fi].status(),
                                                           static_cast<std::int64_t>(fi),
                                                           static_cast<std::int64_t>(prepared.size()),
                                                           pf.split.train_indices.numel(),
                                                           pf.split.test_indices.numel()));
    }
    report.folds.push_back(fold_results[fi].move_value());
  }

  const std::int64_t fold_count = static_cast<std::int64_t>(report.folds.size());
  const std::int64_t metric_count = report.folds.front().metrics.numel();
  for (const auto& fold : report.folds) {
    if (fold.metrics.numel() != metric_count) {
      return StatusOr<CrossAssessReport>(
          Status::internal("cross_assess encountered inconsistent per-fold metric dimensions."));
    }
  }

  Series fold_matrix = report.folds.front().metrics.reshape({1, metric_count});
  for (std::size_t i = 1; i < report.folds.size(); ++i) {
    fold_matrix = fold_matrix.unite({report.folds[i].metrics.reshape({1, metric_count})}, 0);
  }
  fold_matrix = fold_matrix.reshape({fold_count, metric_count});

  report.mean_metrics = fold_matrix.mean(0, false).reshape({metric_count});
  const Series centered = fold_matrix.sub(report.mean_metrics.reshape({1, metric_count}));
  report.std_metrics = centered.square().mean(0, false).sqrt().reshape({metric_count});

  return StatusOr<CrossAssessReport>(std::move(report));
}

} // namespace pluss::ai::nodej
