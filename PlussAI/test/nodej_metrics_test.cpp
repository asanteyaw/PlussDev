#include <pluss/ai/ml.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-8) {
  return std::abs(a - b) <= tol;
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

LabeledBatch make_xor_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -1.0, -1.0,
                                          -1.0,  1.0,
                                           1.0, -1.0,
                                           1.0,  1.0,
                                          -0.9, -0.9,
                                          -0.9,  0.9,
                                           0.9, -0.9,
                                           0.9,  0.9,
                                          -1.2, -1.1,
                                          -1.1,  1.2,
                                           1.1, -1.2,
                                           1.2,  1.1,
                                          -0.4, -0.7,
                                          -0.5,  0.8,
                                           0.6, -0.8,
                                           0.5,  0.7},
                                      {16, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0},
                                      {16},
                                      f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

ModelSpec linear_logistic_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };
}

bool run_nodej_metrics_checks(std::ostream* err = nullptr) {
  {
    const Series truth = pluss::make_series(std::vector<double>{2.0, 0.0, 2.0, 2.0, 0.0, 1.0}, {6}, f64());
    const Series pred = pluss::make_series(std::vector<double>{0.0, 0.0, 2.0, 2.0, 0.0, 2.0}, {6}, f64());

    auto cm_or = confusion_table(truth, pred, {0.0, 1.0, 2.0}, ConfusionNormalize::None);
    if (!cm_or.ok()) {
      if (err) *err << "confusion_table should succeed on valid multiclass labels\n";
      return false;
    }
    const auto cm = as_f64(cm_or.value().counts);
    const std::vector<double> want_counts{2.0, 0.0, 0.0,
                                          0.0, 0.0, 1.0,
                                          1.0, 0.0, 2.0};
    if (cm.size() != want_counts.size()) {
      if (err) *err << "confusion_table should return a 3x3 table\n";
      return false;
    }
    for (std::size_t i = 0; i < cm.size(); ++i) {
      if (!approx(cm[i], want_counts[i])) {
        if (err) *err << "confusion_table counts should match known reference values\n";
        return false;
      }
    }

    auto cm_norm_or = confusion_table(truth, pred, {0.0, 1.0, 2.0}, ConfusionNormalize::ByTruth);
    if (!cm_norm_or.ok()) {
      if (err) *err << "row-normalized confusion_table should succeed\n";
      return false;
    }
    const auto cmn = as_f64(cm_norm_or.value().counts);
    if (!approx(cmn[0], 1.0) || !approx(cmn[5], 1.0) || !approx(cmn[6], 1.0 / 3.0) || !approx(cmn[8], 2.0 / 3.0)) {
      if (err) *err << "row-normalized confusion_table should follow per-row normalization\n";
      return false;
    }

    auto report_or = classification_summary(truth, pred, {0.0, 1.0, 2.0}, 0.0);
    if (!report_or.ok()) {
      if (err) *err << "classification_summary should succeed on valid multiclass labels\n";
      return false;
    }
    const ClassReport& report = report_or.value();
    const auto support = as_f64(report.support);
    const auto precision = as_f64(report.precision);
    const auto recall = as_f64(report.recall);
    const auto f1 = as_f64(report.f1);
    const auto specificity = as_f64(report.specificity);
    const auto geometric = as_f64(report.geometric_mean);
    const auto balance_quality = as_f64(report.balance_quality);
    const auto macro_avg = as_f64(report.macro_average);
    const auto weighted_avg = as_f64(report.weighted_average);
    const auto micro_avg = as_f64(report.micro_average);
    const auto macro_balance = as_f64(report.macro_balance);
    const auto weighted_balance = as_f64(report.weighted_balance);
    const auto accuracy = as_f64(report.accuracy);

    if (support.size() != 3 || !approx(support[0], 2.0) || !approx(support[1], 1.0) || !approx(support[2], 3.0)) {
      if (err) *err << "classification_summary should expose per-class support\n";
      return false;
    }
    if (!approx(precision[0], 2.0 / 3.0) || !approx(precision[1], 0.0) || !approx(precision[2], 2.0 / 3.0)
        || !approx(recall[0], 1.0) || !approx(recall[1], 0.0) || !approx(recall[2], 2.0 / 3.0)
        || !approx(f1[0], 0.8) || !approx(f1[1], 0.0) || !approx(f1[2], 2.0 / 3.0)) {
      if (err) *err << "classification_summary precision/recall/f1 should match known values\n";
      return false;
    }
    if (macro_avg.size() != 3 || !approx(macro_avg[0], 4.0 / 9.0) || !approx(macro_avg[1], 5.0 / 9.0)) {
      if (err) *err << "classification_summary macro averages should match reference values\n";
      return false;
    }
    if (weighted_avg.size() != 3 || !approx(weighted_avg[0], 5.0 / 9.0) || !approx(weighted_avg[1], 2.0 / 3.0)) {
      if (err) *err << "classification_summary weighted averages should match reference values\n";
      return false;
    }
    if (micro_avg.size() != 3 || !approx(micro_avg[0], 2.0 / 3.0) || !approx(micro_avg[1], 2.0 / 3.0)
        || !approx(micro_avg[2], 2.0 / 3.0) || accuracy.size() != 1 || !approx(accuracy[0], 2.0 / 3.0)) {
      if (err) *err << "classification_summary micro average and accuracy should agree for single-label multiclass\n";
      return false;
    }
    if (specificity.size() != 3 || !approx(specificity[0], 0.75) || !approx(specificity[1], 1.0)
        || !approx(specificity[2], 2.0 / 3.0)) {
      if (err) *err << "classification_summary specificity should match one-vs-rest reference values\n";
      return false;
    }
    if (geometric.size() != 3 || !approx(geometric[0], std::sqrt(0.75)) || !approx(geometric[1], 0.0)
        || !approx(geometric[2], 2.0 / 3.0)) {
      if (err) *err << "classification_summary geometric mean should combine sensitivity and specificity\n";
      return false;
    }
    if (balance_quality.size() != 3 || !approx(balance_quality[0], 0.76875) || !approx(balance_quality[1], 0.0)
        || !approx(balance_quality[2], 4.0 / 9.0)) {
      if (err) *err << "classification_summary balance quality should follow the Pluss aggregate formula\n";
      return false;
    }
    if (macro_balance.size() != 3 || !approx(macro_balance[0], 29.0 / 36.0)
        || !approx(macro_balance[1], (std::sqrt(0.75) + 2.0 / 3.0) / 3.0)
        || !approx(macro_balance[2], (0.76875 + 4.0 / 9.0) / 3.0)) {
      if (err) *err << "classification_summary macro balance aggregates should match reference values\n";
      return false;
    }
    if (weighted_balance.size() != 3
        || !approx(weighted_balance[0], (2.0 * 0.75 + 1.0 + 3.0 * (2.0 / 3.0)) / 6.0)) {
      if (err) *err << "classification_summary weighted balance aggregates should use support weights\n";
      return false;
    }

    auto metric_vec_or = classification_metrics(
        truth,
        pred,
        {MetricKind::Accuracy,
         MetricKind::BalancedAccuracy,
         MetricKind::Precision,
         MetricKind::Recall,
         MetricKind::Sensitivity,
         MetricKind::F1,
         MetricKind::Specificity,
         MetricKind::GeometricMean,
         MetricKind::BalanceQuality},
        ClassReportOptions{.zero_division = 0.0});
    if (!metric_vec_or.ok()) {
      if (err) *err << "classification_metrics should succeed on valid hard labels\n";
      return false;
    }
    const auto metrics = as_f64(metric_vec_or.value());
    if (metrics.size() != 9 || !approx(metrics[0], 2.0 / 3.0) || !approx(metrics[1], 5.0 / 9.0)
        || !approx(metrics[2], 4.0 / 9.0) || !approx(metrics[3], 5.0 / 9.0)
        || !approx(metrics[4], 5.0 / 9.0) || !approx(metrics[6], 29.0 / 36.0)) {
      if (err) *err << "classification_metrics should return stable aggregate metric values\n";
      return false;
    }

    const Series weights = pluss::make_series(std::vector<double>{1.0, 3.0, 1.0, 1.0, 2.0, 4.0}, {6}, f64());
    auto weighted_cm_or = confusion_table(
        truth,
        pred,
        ConfusionOptions{.normalize = ConfusionNormalize::None,
                         .label_order = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}, f64()),
                         .sample_weights = weights});
    if (!weighted_cm_or.ok()) {
      if (err) *err << "weighted confusion_table should succeed\n";
      return false;
    }
    const auto weighted_cm = as_f64(weighted_cm_or.value().counts);
    const std::vector<double> want_weighted_counts{5.0, 0.0, 0.0,
                                                   0.0, 0.0, 4.0,
                                                   1.0, 0.0, 2.0};
    if (weighted_cm.size() != want_weighted_counts.size()) {
      if (err) *err << "weighted confusion_table should remain 3x3\n";
      return false;
    }
    for (std::size_t i = 0; i < weighted_cm.size(); ++i) {
      if (!approx(weighted_cm[i], want_weighted_counts[i])) {
        if (err) *err << "weighted confusion_table counts should sum sample weights per cell\n";
        return false;
      }
    }
  }

  {
    const LabeledBatch batch = make_xor_batch();
    SplitOptions split{};
    split.plan = CVPlanKind::CrossVal;
    split.folds = 4;
    split.shuffle = true;
    split.random.seed = 73;
    split.random.deterministic = true;

    TrainOptions train{};
    train.max_iterations = 2500;
    train.tolerance = 1e-8;

    auto fold_scores_or = cross_scores(linear_logistic_spec(), batch, MetricKind::Accuracy, split, train);
    if (!fold_scores_or.ok()) {
      if (err) *err << "cross_scores should succeed for valid setup\n";
      return false;
    }
    const Series fold_scores = fold_scores_or.value().reshape({fold_scores_or.value().numel()});
    if (fold_scores.numel() != split.folds || !fold_scores.isfinite().all().item<bool>()) {
      if (err) *err << "cross_scores should return one finite score per fold\n";
      return false;
    }

    auto gm_scores_or = cross_scores(linear_logistic_spec(), batch, MetricKind::GeometricMean, split, train);
    if (!gm_scores_or.ok()) {
      if (err) *err << "cross_scores should support geometric mean through estimator assess\n";
      return false;
    }
    const Series gm_scores = gm_scores_or.value().reshape({gm_scores_or.value().numel()});
    if (gm_scores.numel() != split.folds || !gm_scores.isfinite().all().item<bool>()) {
      if (err) *err << "geometric mean cross_scores should return one finite score per fold\n";
      return false;
    }

    auto mean_or = cross_score(linear_logistic_spec(), batch, MetricKind::Accuracy, split, train);
    if (!mean_or.ok()) {
      if (err) *err << "cross_score should succeed for valid setup\n";
      return false;
    }

    AssessOptions assess{};
    assess.metrics = {MetricKind::Accuracy};
    auto cv_or = cross_assess(linear_logistic_spec(), batch, split, train, assess);
    if (!cv_or.ok()) {
      if (err) *err << "cross_assess should succeed for parity check\n";
      return false;
    }
    const double mean_from_assess = cv_or.value().mean_metrics.reshape({1}).item<double>();
    if (!approx(mean_or.value(), mean_from_assess, 1e-10)) {
      if (err) *err << "cross_score should equal cross_assess mean metric for same setup\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej metrics suite", "[nodej][metrics]") {
  if (!run_nodej_metrics_checks()) {
    FAIL("nodej metrics checks failed");
  }
}
#else
int main() {
  if (!run_nodej_metrics_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej metrics checks passed\n";
  return 0;
}
#endif
