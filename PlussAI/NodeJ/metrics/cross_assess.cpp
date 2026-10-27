#include "pluss/ai/nodej/metrics.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Config i64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] std::vector<MetricKind> default_metrics(TaskKind task) {
  if (task == TaskKind::Classification) return {MetricKind::Accuracy};
  if (task == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  return {};
}

[[nodiscard]] StatusOr<std::int64_t> metric_index(pluss::msharp::RangeView::Of<MetricKind> metrics,
                                                  MetricKind metric,
                                                  const char* where) {
  for (std::size_t i = 0; i < metrics.size(); ++i) {
    if (metrics[i] == metric) return StatusOr<std::int64_t>(static_cast<std::int64_t>(i));
  }
  return StatusOr<std::int64_t>(
      Status::invalid_argument(std::string(where) + " metric was not requested in AssessOptions.metrics."));
}

[[nodiscard]] bool finite_label_vector(const Series& labels) {
  return labels.isfinite().all().item<bool>();
}

[[nodiscard]] StatusOr<Series> normalize_labels_1d(const Series& labels, const char* where) {
  if (!labels.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires defined label vectors."));
  }
  const Series flat = labels.to(ScalarType::Float64).reshape({labels.numel()});
  if (flat.numel() <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty label vectors."));
  }
  if (!finite_label_vector(flat)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " does not support NaN/Inf labels."));
  }
  return StatusOr<Series>(flat);
}

[[nodiscard]] StatusOr<Series> normalize_sample_weights_1d(const Series& weights,
                                                           std::int64_t expected,
                                                           Platform platform,
                                                           const char* where) {
  if (!weights.defined()) {
    return StatusOr<Series>(pluss::series::ones({expected}, f64_on(platform)));
  }
  const Series flat = weights.to(platform, ScalarType::Float64).reshape({weights.numel()});
  if (flat.numel() != expected) {
    return StatusOr<Series>(
        Status::shape_mismatch(std::string(where) + " sample_weights length must match label length."));
  }
  if (!flat.isfinite().all().item<bool>()) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " sample_weights must be finite."));
  }
  if (!flat.ge(0.0).all().item<bool>()) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " sample_weights must be non-negative."));
  }
  return StatusOr<Series>(flat);
}

[[nodiscard]] StatusOr<Series> resolve_label_order(const Series& truth,
                                                   const Series& pred,
                                                   const Series& label_order,
                                                   const char* where) {
  if (label_order.defined()) {
    StatusOr<Series> order_or = normalize_labels_1d(label_order, where);
    if (!order_or.ok()) return StatusOr<Series>(order_or.status());
    const Series order = order_or.move_value();
    if (order.numel() <= 0) {
      return StatusOr<Series>(Status::invalid_argument(std::string(where) + " label_order must be non-empty."));
    }
    const Series uniq_raw = order.union1d(order, /*sorted=*/false);
    const Series uniq = uniq_raw.reshape({uniq_raw.numel()});
    if (uniq.numel() != order.numel()) {
      return StatusOr<Series>(Status::invalid_argument(std::string(where) + " label_order must not contain duplicates."));
    }
    return StatusOr<Series>(order);
  }
  const Series merged = truth.union1d(pred, /*sorted=*/true);
  return StatusOr<Series>(merged.reshape({merged.numel()}));
}

[[nodiscard]] StatusOr<ConfusionTable> confusion_table_impl(const Series& truth_labels,
                                                            const Series& predicted_labels,
                                                            const ConfusionOptions& options) {
  StatusOr<Series> truth_or = normalize_labels_1d(truth_labels, "confusion_table");
  if (!truth_or.ok()) return StatusOr<ConfusionTable>(truth_or.status());
  StatusOr<Series> pred_or = normalize_labels_1d(predicted_labels, "confusion_table");
  if (!pred_or.ok()) return StatusOr<ConfusionTable>(pred_or.status());

  Series truth = truth_or.move_value();
  Series pred = pred_or.move_value();
  if (truth.numel() != pred.numel()) {
    return StatusOr<ConfusionTable>(
        Status::shape_mismatch("confusion_table requires truth_labels and predicted_labels to have equal length."));
  }

  StatusOr<Series> order_or = resolve_label_order(truth, pred, options.label_order, "confusion_table");
  if (!order_or.ok()) return StatusOr<ConfusionTable>(order_or.status());
  Series labels = order_or.move_value();
  const std::int64_t n = truth.numel();
  const Platform platform = truth.platform();
  labels = labels.to(platform, ScalarType::Float64).reshape({labels.numel()});
  const std::int64_t k = labels.numel();

  const Series truth_match = truth.reshape({n, 1}).eq(labels.reshape({1, k}));
  const Series pred_match = pred.reshape({n, 1}).eq(labels.reshape({1, k}));
  if (!truth_match.any(1, false).all().item<bool>()) {
    return StatusOr<ConfusionTable>(
        Status::invalid_argument("confusion_table truth_labels contains values absent from label_order."));
  }
  if (!pred_match.any(1, false).all().item<bool>()) {
    return StatusOr<ConfusionTable>(
        Status::invalid_argument("confusion_table predicted_labels contains values absent from label_order."));
  }

  StatusOr<Series> weights_or = normalize_sample_weights_1d(options.sample_weights, n, platform, "confusion_table");
  if (!weights_or.ok()) return StatusOr<ConfusionTable>(weights_or.status());
  const Series weights = weights_or.value().reshape({n, 1});
  const Series pred_weighted = pred_match.to(ScalarType::Float64).mul(weights);
  Series counts = truth_match.to(ScalarType::Float64).transpose(0, 1).matmul(pred_weighted);
  counts = counts.reshape({k, k});

  if (options.normalize == ConfusionNormalize::ByTruth) {
    const Series row_sum = counts.sum(1, true);
    const Series row_mask = row_sum.gt(0.0).to(ScalarType::Float64);
    counts = counts.div(row_sum.clamp_min(1.0)).mul(row_mask);
  } else if (options.normalize == ConfusionNormalize::ByPrediction) {
    const Series col_sum = counts.sum(0, true);
    const Series col_mask = col_sum.gt(0.0).to(ScalarType::Float64);
    counts = counts.div(col_sum.clamp_min(1.0)).mul(col_mask);
  } else if (options.normalize == ConfusionNormalize::Global) {
    const Series total = counts.sum().reshape({1, 1});
    const Series total_mask = total.gt(0.0).to(ScalarType::Float64);
    counts = counts.div(total.clamp_min(1.0)).mul(total_mask);
  }

  ConfusionTable out{};
  out.labels = labels.to(platform, ScalarType::Float64).reshape({k});
  out.counts = counts.to(platform, ScalarType::Float64).reshape({k, k});
  out.normalize = options.normalize;
  return StatusOr<ConfusionTable>(std::move(out));
}

[[nodiscard]] Series safe_ratio(const Series& numer, const Series& denom, double zero_division) {
  const Series denom_f = denom.to(ScalarType::Float64);
  const Series numer_f = numer.to(ScalarType::Float64);
  const Series mask = denom_f.gt(0.0).to(ScalarType::Float64);
  const Series fallback = denom_f.mul(0.0).add(zero_division);
  return numer_f.div(denom_f.clamp_min(1.0)).mul(mask).add(fallback.mul(mask.mul(-1.0).add(1.0)));
}

// Like safe_ratio but for real-valued (possibly fractional, possibly negative)
// numerators/denominators: returns numer/denom where denom != 0, and 0.0 where
// denom == 0. Used for MCC and Cohen's kappa, whose denominators are variances
// / (1 - p_e) rather than integer counts, so safe_ratio's clamp_min(1.0) would
// corrupt them.
[[nodiscard]] Series guarded_ratio(const Series& numer, const Series& denom) {
  const Series denom_f = denom.to(ScalarType::Float64);
  const Series numer_f = numer.to(ScalarType::Float64);
  const Series nonzero = denom_f.gt(0.0).to(ScalarType::Float64);      // 1 where denom > 0, else 0
  const Series safe_den = denom_f.add(nonzero.mul(-1.0).add(1.0));     // denom where > 0, else 1
  return numer_f.div(safe_den).mul(nonzero);
}

[[nodiscard]] Series balance_quality_score(const Series& sensitivity,
                                           const Series& specificity,
                                           double alpha,
                                           bool square_base) {
  const Series gm = sensitivity.mul(specificity).clamp_min(0.0).sqrt();
  const Series base = square_base ? gm.square() : gm;
  return base.mul(sensitivity.sub(specificity).mul(alpha).add(1.0)).clamp_min(0.0);
}

[[nodiscard]] StatusOr<ClassReport> classification_summary_impl(const Series& truth_labels,
                                                                const Series& predicted_labels,
                                                                const ClassReportOptions& options) {
  if (!(options.zero_division == 0.0 || options.zero_division == 1.0) || !std::isfinite(options.zero_division)) {
    return StatusOr<ClassReport>(
        Status::invalid_argument("classification_summary zero_division must be finite and equal to 0.0 or 1.0."));
  }
  if (!std::isfinite(options.balance_quality_alpha) || options.balance_quality_alpha < 0.0) {
    return StatusOr<ClassReport>(
        Status::invalid_argument("classification_summary balance_quality_alpha must be finite and non-negative."));
  }

  StatusOr<ConfusionTable> table_or =
      confusion_table_impl(truth_labels, predicted_labels, ConfusionOptions{.normalize = ConfusionNormalize::None,
                                                                             .label_order = options.label_order,
                                                                             .sample_weights = options.sample_weights});
  if (!table_or.ok()) return StatusOr<ClassReport>(table_or.status());
  const ConfusionTable& table = table_or.value();

  const Series counts = table.counts.to(ScalarType::Float64);
  const Platform platform = counts.platform();
  const std::int64_t k = counts.sizes()[0];
  const Series support = counts.sum(1, false).reshape({k});
  const Series pred_support = counts.sum(0, false).reshape({k});
  const Series tp = counts.diagonal().reshape({k});
  const Series precision = safe_ratio(tp, pred_support, options.zero_division).reshape({k});
  const Series recall = safe_ratio(tp, support, options.zero_division).reshape({k});
  const Series f1 = safe_ratio(precision.mul(recall).mul(2.0), precision.add(recall), options.zero_division).reshape({k});

  const Series total = support.sum().reshape({1});
  const Series fp = pred_support.sub(tp).reshape({k});
  const Series tn = total.sub(support.add(pred_support).sub(tp)).reshape({k});
  const Series specificity = safe_ratio(tn, tn.add(fp), options.zero_division).reshape({k});
  const Series geometric_mean = recall.mul(specificity).clamp_min(0.0).sqrt().reshape({k});
  const Series balance_quality = balance_quality_score(recall,
                                                       specificity,
                                                       options.balance_quality_alpha,
                                                       options.square_balance_quality).reshape({k});

  const Series tp_sum = tp.sum().reshape({1});
  const Series fp_sum = pred_support.sum().reshape({1}).sub(tp_sum);
  const Series fn_sum = support.sum().reshape({1}).sub(tp_sum);
  const Series tn_sum = tn.sum().reshape({1});

  const Series micro_p = safe_ratio(tp_sum, tp_sum.add(fp_sum), options.zero_division).reshape({1});
  const Series micro_r = safe_ratio(tp_sum, tp_sum.add(fn_sum), options.zero_division).reshape({1});
  const Series micro_f1 = safe_ratio(micro_p.mul(micro_r).mul(2.0), micro_p.add(micro_r), options.zero_division).reshape({1});
  const Series micro_s = safe_ratio(tn_sum, tn_sum.add(fp_sum), options.zero_division).reshape({1});
  const Series micro_gm = micro_r.mul(micro_s).clamp_min(0.0).sqrt().reshape({1});
  const Series micro_bq = balance_quality_score(micro_r,
                                                micro_s,
                                                options.balance_quality_alpha,
                                                options.square_balance_quality).reshape({1});

  const Series accuracy = safe_ratio(tp_sum, total, options.zero_division).reshape({1});

  // Matthews correlation coefficient and unweighted Cohen's kappa, both from
  // the confusion matrix. p_k = pred_support (times class k predicted),
  // t_k = support (times class k truly occurred), s = total, c = tp_sum.
  const Series s_sq = total.mul(total).reshape({1});
  const Series dot_pt = pred_support.mul(support).sum().reshape({1});   // Sum_k p_k * t_k
  const Series dot_pp = pred_support.mul(pred_support).sum().reshape({1});
  const Series dot_tt = support.mul(support).sum().reshape({1});
  const Series mcc_num = tp_sum.mul(total).sub(dot_pt).reshape({1});
  const Series mcc_den = s_sq.sub(dot_pp).mul(s_sq.sub(dot_tt)).clamp_min(0.0).sqrt().reshape({1});
  const Series mcc = guarded_ratio(mcc_num, mcc_den).reshape({1});     // 0 when the variance product is 0
  const Series p_e = guarded_ratio(dot_pt, s_sq).reshape({1});         // expected agreement by chance
  const Series kappa = guarded_ratio(accuracy.sub(p_e), p_e.mul(-1.0).add(1.0)).reshape({1});  // (p_o - p_e)/(1 - p_e)

  const Series macro_p = precision.mean().reshape({1});
  const Series macro_r = recall.mean().reshape({1});
  const Series macro_f1 = f1.mean().reshape({1});
  const Series macro_s = specificity.mean().reshape({1});
  const Series macro_gm = geometric_mean.mean().reshape({1});
  const Series macro_bq = balance_quality.mean().reshape({1});

  const Series weight = safe_ratio(support, total, 0.0).reshape({k});
  const Series weighted_p = precision.mul(weight).sum().reshape({1});
  const Series weighted_r = recall.mul(weight).sum().reshape({1});
  const Series weighted_f1 = f1.mul(weight).sum().reshape({1});
  const Series weighted_s = specificity.mul(weight).sum().reshape({1});
  const Series weighted_gm = geometric_mean.mul(weight).sum().reshape({1});
  const Series weighted_bq = balance_quality.mul(weight).sum().reshape({1});

  ClassReport out{};
  out.labels = table.labels.to(platform, ScalarType::Float64).reshape({k});
  out.support = support.to(platform, ScalarType::Float64).reshape({k});
  out.precision = precision.to(platform, ScalarType::Float64).reshape({k});
  out.recall = recall.to(platform, ScalarType::Float64).reshape({k});
  out.f1 = f1.to(platform, ScalarType::Float64).reshape({k});
  out.specificity = specificity.to(platform, ScalarType::Float64).reshape({k});
  out.geometric_mean = geometric_mean.to(platform, ScalarType::Float64).reshape({k});
  out.balance_quality = balance_quality.to(platform, ScalarType::Float64).reshape({k});
  out.accuracy = accuracy.to(platform, ScalarType::Float64).reshape({1});
  out.mcc = mcc.to(platform, ScalarType::Float64).reshape({1});
  out.cohen_kappa = kappa.to(platform, ScalarType::Float64).reshape({1});
  out.micro_average = micro_p.unite({micro_r, micro_f1}, 0).reshape({3});
  out.macro_average = macro_p.unite({macro_r, macro_f1}, 0).reshape({3});
  out.weighted_average = weighted_p.unite({weighted_r, weighted_f1}, 0).reshape({3});
  out.micro_balance = micro_s.unite({micro_gm, micro_bq}, 0).reshape({3});
  out.macro_balance = macro_s.unite({macro_gm, macro_bq}, 0).reshape({3});
  out.weighted_balance = weighted_s.unite({weighted_gm, weighted_bq}, 0).reshape({3});
  return StatusOr<ClassReport>(std::move(out));
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
  if (split.plan == CVPlanKind::CrossVal) {
    if (split.stratified && spec.task == TaskKind::Classification) {
      return stratified_crossval_plan(batch.y, split);
    }
    return crossval_plan(batch.features.sample_count(), split);
  }
  if (split.plan == CVPlanKind::StratifiedCrossVal) {
    if (spec.task != TaskKind::Classification) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("cross_assess StratifiedCrossVal requires TaskKind::Classification."));
    }
    return stratified_crossval_plan(batch.y, split);
  }
  if (split.plan == CVPlanKind::TSCrossVal) {
    return ts_crossval_plan(batch.features.sample_count(), split);
  }
  if (split.plan == CVPlanKind::GroupCrossVal) {
    if (!split.groups.defined()) {
      return StatusOr<SplitPlan>(
          Status::invalid_argument("cross_assess GroupCrossVal requires SplitOptions.groups to be defined."));
    }
    if (split.groups.numel() != batch.features.sample_count()) {
      return StatusOr<SplitPlan>(
          Status::shape_mismatch("cross_assess GroupCrossVal requires SplitOptions.groups length to match sample count."));
    }
    return group_crossval_plan(split.groups, split);
  }
  return StatusOr<SplitPlan>(Status::invalid_argument("cross_assess received unknown SplitOptions.plan."));
}

[[nodiscard]] SplitOptions resolved_split_defaults(const ModelSpec& spec, SplitOptions split) {
  if (!split.reliability_defaults) return split;
  if (spec.task != TaskKind::Classification) return split;
  if (!split.stratified
      && (split.plan == CVPlanKind::Holdout || split.plan == CVPlanKind::CrossVal)) {
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
  const auto fit_start = std::chrono::steady_clock::now();
  Status learn_status = model.learn(train, train_options);
  const auto fit_end = std::chrono::steady_clock::now();
  if (!learn_status.ok()) return StatusOr<FoldAssessResult>(std::move(learn_status));
  const auto score_start = std::chrono::steady_clock::now();
  StatusOr<Series> fold_metrics_or = model.assess(test, assess_options);
  const auto score_end = std::chrono::steady_clock::now();
  if (!fold_metrics_or.ok()) return StatusOr<FoldAssessResult>(fold_metrics_or.status());
  Series fold_metrics = fold_metrics_or.move_value().to(ScalarType::Float64);
  FoldAssessResult fold{};
  fold.metrics = fold_metrics.reshape({fold_metrics.numel()});
  fold.train_indices = split.train_indices;
  fold.test_indices = split.test_indices;
  fold.fit_time_seconds = std::chrono::duration<double>(fit_end - fit_start).count();
  fold.score_time_seconds = std::chrono::duration<double>(score_end - score_start).count();
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

StatusOr<Series> cross_scores(const ModelSpec& spec,
                              const LabeledBatch& batch,
                              MetricKind metric,
                              const SplitOptions& split,
                              const TrainOptions& train_options,
                              const AssessOptions& assess_options) {
  AssessOptions effective_assess = assess_options;
  if (effective_assess.metrics.empty()) {
    effective_assess.metrics = {metric};
  } else {
    const auto it = std::find(effective_assess.metrics.begin(), effective_assess.metrics.end(), metric);
    if (it == effective_assess.metrics.end()) effective_assess.metrics.push_back(metric);
  }

  StatusOr<CrossAssessReport> report_or = cross_assess(spec, batch, split, train_options, effective_assess);
  if (!report_or.ok()) return StatusOr<Series>(report_or.status());
  const CrossAssessReport& report = report_or.value();
  if (report.folds.empty()) return StatusOr<Series>(Status::internal("cross_scores requires at least one fold."));

  StatusOr<std::int64_t> metric_pos_or =
      metric_index(pluss::msharp::RangeView::Of<MetricKind>(report.metrics), metric, "cross_scores");
  if (!metric_pos_or.ok()) return StatusOr<Series>(metric_pos_or.status());
  const std::int64_t metric_pos = metric_pos_or.value();

  const std::int64_t fold_count = static_cast<std::int64_t>(report.folds.size());
  const std::int64_t metric_count = report.folds.front().metrics.numel();
  for (const auto& fold : report.folds) {
    if (fold.metrics.numel() != metric_count) {
      return StatusOr<Series>(Status::internal("cross_scores encountered inconsistent per-fold metric shapes."));
    }
  }

  Series fold_matrix = report.folds.front().metrics.reshape({1, metric_count});
  for (std::size_t i = 1; i < report.folds.size(); ++i) {
    fold_matrix = fold_matrix.unite({report.folds[i].metrics.reshape({1, metric_count})}, 0);
  }
  fold_matrix = fold_matrix.reshape({fold_count, metric_count});

  const Series metric_index_series = pluss::make_series(
      std::vector<std::int64_t>{metric_pos},
      {1},
      i64_on(fold_matrix.platform()));
  const Series picked = fold_matrix.index_select(1, metric_index_series).reshape({fold_count});
  return StatusOr<Series>(picked.to(fold_matrix.platform(), ScalarType::Float64));
}

StatusOr<double> cross_score(const ModelSpec& spec,
                             const LabeledBatch& batch,
                             MetricKind metric,
                             const SplitOptions& split,
                             const TrainOptions& train_options,
                             const AssessOptions& assess_options) {
  StatusOr<Series> scores_or = cross_scores(spec, batch, metric, split, train_options, assess_options);
  if (!scores_or.ok()) return StatusOr<double>(scores_or.status());
  Series scores = scores_or.move_value();
  scores = scores.reshape({scores.numel()});
  if (scores.numel() <= 0) return StatusOr<double>(Status::internal("cross_score received an empty fold score vector."));
  const double mean = scores.mean().reshape({1}).host<double>().front();
  if (!std::isfinite(mean)) return StatusOr<double>(Status::numerical_failure("cross_score produced a non-finite value."));
  return StatusOr<double>(mean);
}

StatusOr<ConfusionTable> confusion_table(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         const ConfusionOptions& options) {
  return confusion_table_impl(truth_labels, predicted_labels, options);
}

StatusOr<ConfusionTable> confusion_table(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         pluss::msharp::RangeView::Of<double> label_order,
                                         ConfusionNormalize normalize) {
  const Config cfg = truth_labels.defined()
                         ? f64_on(truth_labels.platform())
                         : Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series order = pluss::make_series(std::vector<double>(label_order.begin(), label_order.end()),
                                          {static_cast<std::int64_t>(label_order.size())},
                                          cfg);
  return confusion_table_impl(truth_labels,
                              predicted_labels,
                              ConfusionOptions{
                                  .normalize = normalize,
                                  .label_order = order,
                              });
}

StatusOr<ClassReport> classification_summary(const Series& truth_labels,
                                             const Series& predicted_labels,
                                             const ClassReportOptions& options) {
  return classification_summary_impl(truth_labels, predicted_labels, options);
}

StatusOr<ClassReport> classification_summary(const Series& truth_labels,
                                             const Series& predicted_labels,
                                             pluss::msharp::RangeView::Of<double> label_order,
                                             double zero_division) {
  const Config cfg = truth_labels.defined()
                         ? f64_on(truth_labels.platform())
                         : Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series order = pluss::make_series(std::vector<double>(label_order.begin(), label_order.end()),
                                          {static_cast<std::int64_t>(label_order.size())},
                                          cfg);
  return classification_summary_impl(truth_labels,
                                     predicted_labels,
                                     ClassReportOptions{
                                         .label_order = order,
                                         .zero_division = zero_division,
                                     });
}

StatusOr<Series> classification_metrics(const Series& truth_labels,
                                        const Series& predicted_labels,
                                        const std::vector<MetricKind>& metrics,
                                        const ClassReportOptions& options) {
  std::vector<MetricKind> requested = metrics;
  if (requested.empty()) {
    requested = {MetricKind::Accuracy, MetricKind::BalancedAccuracy, MetricKind::Precision, MetricKind::Recall, MetricKind::F1};
  }

  StatusOr<ClassReport> report_or = classification_summary_impl(truth_labels, predicted_labels, options);
  if (!report_or.ok()) return StatusOr<Series>(report_or.status());
  const ClassReport& report = report_or.value();
  const Platform platform = report.accuracy.platform();
  const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_on(platform));
  const Series idx1 = pluss::make_series(std::vector<std::int64_t>{1}, {1}, i64_on(platform));
  const Series idx2 = pluss::make_series(std::vector<std::int64_t>{2}, {1}, i64_on(platform));

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(report.accuracy.reshape({1}));
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(report.macro_average.index_select(0, idx1).reshape({1}));
        break;
      case MetricKind::Precision:
        out.push_back(report.macro_average.index_select(0, idx0).reshape({1}));
        break;
      case MetricKind::Recall:
      case MetricKind::Sensitivity:
        out.push_back(report.macro_average.index_select(0, idx1).reshape({1}));
        break;
      case MetricKind::F1:
        out.push_back(report.macro_average.index_select(0, idx2).reshape({1}));
        break;
      case MetricKind::Specificity:
        out.push_back(report.macro_balance.index_select(0, idx0).reshape({1}));
        break;
      case MetricKind::GeometricMean:
        out.push_back(report.macro_balance.index_select(0, idx1).reshape({1}));
        break;
      case MetricKind::BalanceQuality:
        out.push_back(report.macro_balance.index_select(0, idx2).reshape({1}));
        break;
      case MetricKind::Mcc:
        out.push_back(report.mcc.reshape({1}));
        break;
      case MetricKind::CohenKappa:
        out.push_back(report.cohen_kappa.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument(
            "classification_metrics from hard labels supports hard-label classification metrics only. "
            "Use classifier.assess(...) for probability-driven metrics (RocAuc, LogLoss, Gini, "
            "AveragePrecision, Brier)."));
    }
  }
  if (out.empty()) {
    return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(platform)));
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(platform, ScalarType::Float64));
}

StatusOr<Series> regression_metrics(const Series& truth, const Series& prediction, const std::vector<MetricKind>& metrics) {
  if (!truth.defined() || !prediction.defined()) {
    return StatusOr<Series>(Status::invalid_argument("regression_metrics requires defined truth/prediction vectors."));
  }
  const Series y = truth.to(ScalarType::Float64).reshape({truth.numel()});
  const Series yhat = prediction.to(ScalarType::Float64).reshape({prediction.numel()});
  if (y.numel() != yhat.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("regression_metrics requires equal truth/prediction lengths."));
  }
  if (y.numel() == 0) {
    return StatusOr<Series>(Status::invalid_argument("regression_metrics requires at least one sample."));
  }

  std::vector<MetricKind> requested = metrics;
  if (requested.empty()) requested = default_metrics(TaskKind::Regression);

  const Platform platform = y.platform();
  const Series err = yhat.sub(y);
  const Series mse = err.square().mean();
  const Series mae = err.abs().mean();
  const Series sse = err.square().sum();
  const Series sst = y.sub(y.mean()).square().sum();
  const Series one = pluss::series::ones({1}, f64_on(platform));
  const Series zero = pluss::series::zeros({1}, f64_on(platform));
  const Series r2 = sst.gt(std::numeric_limits<double>::epsilon()).where(one.sub(sse.div(sst)), zero);

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Mse:
        out.push_back(mse.reshape({1}));
        break;
      case MetricKind::Rmse:
        out.push_back(mse.sqrt().reshape({1}));
        break;
      case MetricKind::Mae:
        out.push_back(mae.reshape({1}));
        break;
      case MetricKind::R2:
        out.push_back(r2.reshape({1}));
        break;
      default:
        return StatusOr<Series>(
            Status::invalid_argument("regression_metrics supports Mse/Rmse/Mae/R2 only."));
    }
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(platform, ScalarType::Float64));
}

StatusOr<ConfusionTable> confusion_table(const ClassifierBase& classifier,
                                         const LabeledBatch& batch,
                                         const InferOptions& infer_options,
                                         const ConfusionOptions& options) {
  Status check = validate_labeled(batch);
  if (!check.ok()) return StatusOr<ConfusionTable>(std::move(check));
  StatusOr<Series> pred_or = classifier.infer(batch.features, infer_options);
  if (!pred_or.ok()) return StatusOr<ConfusionTable>(pred_or.status());
  return confusion_table_impl(batch.y, pred_or.move_value(), options);
}

StatusOr<ClassReport> classification_summary(const ClassifierBase& classifier,
                                             const LabeledBatch& batch,
                                             const InferOptions& infer_options,
                                             const ClassReportOptions& options) {
  Status check = validate_labeled(batch);
  if (!check.ok()) return StatusOr<ClassReport>(std::move(check));
  StatusOr<Series> pred_or = classifier.infer(batch.features, infer_options);
  if (!pred_or.ok()) return StatusOr<ClassReport>(pred_or.status());
  return classification_summary_impl(batch.y, pred_or.move_value(), options);
}

StatusOr<Series> classification_metrics(const ClassifierBase& classifier,
                                        const LabeledBatch& batch,
                                        const AssessOptions& options) {
  Status check = validate_labeled(batch);
  if (!check.ok()) return StatusOr<Series>(std::move(check));
  return classifier.assess(batch, options);
}

} // namespace pluss::ai::nodej
