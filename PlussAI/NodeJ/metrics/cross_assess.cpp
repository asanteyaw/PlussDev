#include "pluss/ai/nodej/metrics.h"

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

[[nodiscard]] StatusOr<FeatureBatch> slice_features(const FeatureBatch& in, const Series& indices) {
  Status check = validate_features(in);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));

  const Series idx = indices.to(in.x.platform(), ScalarType::Int64).reshape({indices.numel()});
  FeatureBatch out = in;
  if (in.layout == ObservationLayout::Rows) {
    out.x = in.x.index_select(0, idx);
  } else {
    out.x = in.x.index_select(1, idx);
  }
  return StatusOr<FeatureBatch>(std::move(out));
}

[[nodiscard]] StatusOr<LabeledBatch> slice_labeled(const LabeledBatch& in, const Series& indices) {
  Status check = validate_labeled(in);
  if (!check.ok()) return StatusOr<LabeledBatch>(std::move(check));

  StatusOr<FeatureBatch> features_or = slice_features(in.features, indices);
  if (!features_or.ok()) return StatusOr<LabeledBatch>(features_or.status());

  const Series y_idx = indices.to(in.y.platform(), ScalarType::Int64).reshape({indices.numel()});
  LabeledBatch out{};
  out.features = features_or.move_value();
  out.y = in.y.reshape({in.y.numel()}).index_select(0, y_idx);
  if (in.sample_weights.defined()) {
    const Series w_idx = indices.to(in.sample_weights.platform(), ScalarType::Int64).reshape({indices.numel()});
    out.sample_weights = in.sample_weights.reshape({in.sample_weights.numel()}).index_select(0, w_idx);
  }
  return StatusOr<LabeledBatch>(std::move(out));
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
  if (split.plan == CVPlanKind::GroupKFold || split.plan == CVPlanKind::TimeSeries) {
    return StatusOr<SplitPlan>(Status::not_implemented("cross_assess currently supports Holdout, KFold, StratifiedKFold."));
  }
  return StatusOr<SplitPlan>(Status::invalid_argument("cross_assess received unknown SplitOptions.plan."));
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

  StatusOr<SplitPlan> plan_or = make_cross_plan(spec, batch, split);
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

  for (const auto& fold : plan.folds) {
    StatusOr<LabeledBatch> train_or = slice_labeled(batch, fold.train_indices);
    if (!train_or.ok()) return StatusOr<CrossAssessReport>(train_or.status());
    StatusOr<LabeledBatch> test_or = slice_labeled(batch, fold.test_indices);
    if (!test_or.ok()) return StatusOr<CrossAssessReport>(test_or.status());

    if (spec.task == TaskKind::Classification) {
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
      if (!model_or.ok()) return StatusOr<CrossAssessReport>(model_or.status());
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      StatusOr<FoldAssessResult> fold_or =
          learn_and_assess_fold(*model, train_or.value(), test_or.value(), train_options, fold_assess_options, fold);
      if (!fold_or.ok()) return StatusOr<CrossAssessReport>(fold_or.status());
      report.folds.push_back(fold_or.move_value());
    } else {
      StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
      if (!model_or.ok()) return StatusOr<CrossAssessReport>(model_or.status());
      std::unique_ptr<RegressorBase> model = model_or.move_value();
      StatusOr<FoldAssessResult> fold_or =
          learn_and_assess_fold(*model, train_or.value(), test_or.value(), train_options, fold_assess_options, fold);
      if (!fold_or.ok()) return StatusOr<CrossAssessReport>(fold_or.status());
      report.folds.push_back(fold_or.move_value());
    }
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
