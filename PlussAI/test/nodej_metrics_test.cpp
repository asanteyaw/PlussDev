#include <pluss/ai/ml.h>
#include <sstream>

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

    // Confusion matrix C = [[2,0,0],[0,0,1],[1,0,2]] -> p_k = [3,0,3], t_k = [2,1,3],
    // c = 4, s = 6. MCC = (4*6 - 15) / sqrt((36-18)(36-14)) = 9 / sqrt(396) = 3*sqrt(11)/22.
    // kappa: p_o = 2/3, p_e = 15/36 = 5/12 -> (2/3 - 5/12)/(1 - 5/12) = (3/12)/(7/12) = 3/7.
    const auto mcc = as_f64(report.mcc);
    const auto cohen_kappa = as_f64(report.cohen_kappa);
    if (mcc.size() != 1 || !approx(mcc[0], 3.0 * std::sqrt(11.0) / 22.0, 1e-9) || cohen_kappa.size() != 1 ||
        !approx(cohen_kappa[0], 3.0 / 7.0, 1e-9)) {
      if (err) *err << "classification_summary MCC / Cohen's kappa should match the hand-computed reference\n";
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
         MetricKind::BalanceQuality,
         MetricKind::Mcc,
         MetricKind::CohenKappa},
        ClassReportOptions{.zero_division = 0.0});
    if (!metric_vec_or.ok()) {
      if (err) *err << "classification_metrics should succeed on valid hard labels\n";
      return false;
    }
    const auto metrics = as_f64(metric_vec_or.value());
    if (metrics.size() != 11 || !approx(metrics[0], 2.0 / 3.0) || !approx(metrics[1], 5.0 / 9.0)
        || !approx(metrics[2], 4.0 / 9.0) || !approx(metrics[3], 5.0 / 9.0)
        || !approx(metrics[4], 5.0 / 9.0) || !approx(metrics[6], 29.0 / 36.0)
        || !approx(metrics[9], 3.0 * std::sqrt(11.0) / 22.0, 1e-9) || !approx(metrics[10], 3.0 / 7.0, 1e-9)) {
      if (err) *err << "classification_metrics should return stable aggregate metric values (incl. MCC / kappa)\n";
      return false;
    }

    // Score-only metrics must be rejected on the hard-label path, not silently wrong.
    if (classification_metrics(truth, pred, {MetricKind::Gini}, ClassReportOptions{.zero_division = 0.0}).ok() ||
        classification_metrics(truth, pred, {MetricKind::AveragePrecision}, ClassReportOptions{.zero_division = 0.0}).ok() ||
        classification_metrics(truth, pred, {MetricKind::Brier}, ClassReportOptions{.zero_division = 0.0}).ok()) {
      if (err) *err << "classification_metrics from hard labels should reject Gini / AveragePrecision / Brier\n";
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

  {
    // Binary MCC / Cohen's kappa against a hand-computed 2x2: TP=3, FN=1, TN=2, FP=2.
    const Series bt = pluss::make_series(std::vector<double>{1, 1, 1, 1, 0, 0, 0, 0}, {8}, f64());
    const Series bp = pluss::make_series(std::vector<double>{1, 1, 1, 0, 0, 0, 1, 1}, {8}, f64());
    auto br_or = classification_summary(bt, bp, {0.0, 1.0}, 0.0);
    if (!br_or.ok()) {
      if (err) *err << "classification_summary should succeed on binary labels\n";
      return false;
    }
    // MCC = (3*2 - 2*1) / sqrt((3+2)(3+1)(2+2)(2+1)) = 4 / sqrt(240).
    // kappa: p_o = 5/8, p_e = (5*4 + 3*4)/64 = 0.5 -> (0.625 - 0.5)/0.5 = 0.25.
    if (!approx(as_f64(br_or.value().mcc)[0], 4.0 / std::sqrt(240.0), 1e-9) ||
        !approx(as_f64(br_or.value().cohen_kappa)[0], 0.25, 1e-9)) {
      if (err) *err << "binary classification_summary MCC / kappa should match the hand-computed reference\n";
      return false;
    }
    // Degenerate: predict a single class -> the MCC and kappa denominators vanish, both return 0.
    const Series bp0 = pluss::make_series(std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0}, {8}, f64());
    auto d_or = classification_summary(bt, bp0, {0.0, 1.0}, 0.0);
    if (!d_or.ok() || !approx(as_f64(d_or.value().mcc)[0], 0.0) ||
        !approx(as_f64(d_or.value().cohen_kappa)[0], 0.0)) {
      if (err) *err << "single-class predictions should give MCC = 0 and kappa = 0, not NaN\n";
      return false;
    }
  }

  {
    // Hand-computed reference: 5 samples, distinct scores, 3 positives.
    // tp/fp accumulate one at a time walking the sorted list, so every
    // point below was checked by hand against tp/(tp+fp) and tp/positives.
    const Series scores = pluss::make_series(std::vector<double>{0.9, 0.8, 0.7, 0.6, 0.5}, {5}, f64());
    const Series labels = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 1.0, 0.0}, {5}, f64());

    auto curve_or = threshold_curve(scores, labels);
    if (!curve_or.ok()) {
      if (err) *err << "threshold_curve should succeed on a valid binary score/label pair\n";
      return false;
    }
    const auto thresholds = as_f64(curve_or.value().thresholds);
    const auto precision = as_f64(curve_or.value().precision);
    const auto recall = as_f64(curve_or.value().recall);
    const auto f1 = as_f64(curve_or.value().f1);

    const std::vector<double> want_thresholds{0.9, 0.8, 0.7, 0.6, 0.5};
    const std::vector<double> want_precision{1.0, 0.5, 2.0 / 3.0, 0.75, 0.6};
    const std::vector<double> want_recall{1.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0, 1.0, 1.0};
    const std::vector<double> want_f1{0.5, 0.4, 2.0 / 3.0, 6.0 / 7.0, 0.75};

    if (thresholds.size() != want_thresholds.size()) {
      if (err) *err << "threshold_curve should report one point per distinct score\n";
      return false;
    }
    for (std::size_t i = 0; i < thresholds.size(); ++i) {
      if (!approx(thresholds[i], want_thresholds[i], 1e-9) || !approx(precision[i], want_precision[i], 1e-9) ||
          !approx(recall[i], want_recall[i], 1e-9) || !approx(f1[i], want_f1[i], 1e-9)) {
        if (err) *err << "threshold_curve values should match hand-computed reference at index " << i << "\n";
        return false;
      }
    }

    auto best_f1_or = best_threshold(scores, labels, MetricKind::F1);
    if (!best_f1_or.ok() || !approx(best_f1_or.value(), 0.6)) {
      if (err) *err << "best_threshold(F1) should pick the highest-F1 score (0.6)\n";
      return false;
    }
    auto best_precision_or = best_threshold(scores, labels, MetricKind::Precision);
    if (!best_precision_or.ok() || !approx(best_precision_or.value(), 0.9)) {
      if (err) *err << "best_threshold(Precision) should pick the highest-precision score (0.9)\n";
      return false;
    }

    // Average precision = Sum (R_n - R_{n-1}) * P_n over the boundary points above:
    // (1/3)(1) + 0 + (1/3)(2/3) + (1/3)(0.75) + 0 = 1/3 + 2/9 + 1/4 = 29/36.
    auto ap_or = average_precision(scores, labels);
    if (!ap_or.ok() || !approx(ap_or.value(), 29.0 / 36.0, 1e-9)) {
      if (err) *err << "average_precision should equal the hand-computed PR step-sum (29/36)\n";
      return false;
    }
    const Series first_three = scores.slice(0, 0, 3);
    const Series no_positives = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0}, {3}, f64());
    auto ap_neg_or = average_precision(first_three, no_positives);
    if (!ap_neg_or.ok() || !approx(ap_neg_or.value(), 0.0)) {
      if (err) *err << "average_precision with no positive labels should be 0\n";
      return false;
    }

    // Ties collapse onto their last (lowest-recall) position: two samples
    // sharing score 0.5 must report one threshold point, not two.
    const Series tied_scores = pluss::make_series(std::vector<double>{0.9, 0.5, 0.5, 0.1}, {4}, f64());
    const Series tied_labels = pluss::make_series(std::vector<double>{1.0, 1.0, 0.0, 0.0}, {4}, f64());
    auto tied_curve_or = threshold_curve(tied_scores, tied_labels);
    if (!tied_curve_or.ok() || tied_curve_or.value().thresholds.numel() != 3) {
      if (err) *err << "threshold_curve should collapse tied scores onto a single threshold point\n";
      return false;
    }
  }

  {
    // score_metrics from a raw (probabilities, labels) pair -- no model.
    // Binary fixture: p = [.9 .8 .7 .6 .5], y = [1 0 1 1 0].
    //   ROC-AUC = 4 concordant of 6 pos/neg pairs      = 2/3
    //   Gini    = 2*AUC - 1                             = 1/3
    //   AP      = (1/3)(1) + (1/3)(2/3) + (1/3)(3/4)    = 29/36
    //   Brier   = mean (p - y)^2 = 1.15 / 5             = 0.23
    //   LogLoss = -mean[ y ln p + (1-y) ln(1-p) ]       ~ 0.65508924
    //   F1      @ 0.5 threshold: all 5 predicted +, TP=3 FP=2 -> 2*.6*1/1.6 = 0.75
    const Series p = pluss::make_series(std::vector<double>{0.9, 0.8, 0.7, 0.6, 0.5}, {5}, f64());
    const Series y = pluss::make_series(std::vector<double>{1.0, 0.0, 1.0, 1.0, 0.0}, {5}, f64());
    const std::vector<MetricKind> want{MetricKind::RocAuc, MetricKind::Gini, MetricKind::AveragePrecision,
                                       MetricKind::Brier, MetricKind::LogLoss, MetricKind::F1};
    auto sm_or = score_metrics(p, y, want);
    if (!sm_or.ok()) {
      if (err) *err << "score_metrics should succeed on a valid binary probability/label pair\n";
      return false;
    }
    const auto sm = as_f64(sm_or.value());
    // LogLoss = 3.275446176356595 / 5 = 0.655089235271319.
    if (sm.size() != 6 || !approx(sm[0], 2.0 / 3.0, 1e-9) || !approx(sm[1], 1.0 / 3.0, 1e-9) ||
        !approx(sm[2], 29.0 / 36.0, 1e-9) || !approx(sm[3], 0.23, 1e-9) ||
        !approx(sm[4], 0.655089235271319, 1e-9) || !approx(sm[5], 0.75, 1e-9)) {
      if (err) *err << "score_metrics binary values should match the hand-computed reference\n";
      return false;
    }

    // Regression MetricKinds are rejected on a classification score/label pair.
    auto sm_reg_or = score_metrics(p, y, {MetricKind::Mse});
    if (sm_reg_or.ok()) {
      if (err) *err << "score_metrics should reject a regression MetricKind\n";
      return false;
    }

    // Multiclass: 3-class probability matrix, argmax picks the right class on
    // every row so accuracy is 1 and macro one-vs-rest AUC is 1.
    const Series probs3 = pluss::make_series(
        std::vector<double>{0.8, 0.1, 0.1, 0.1, 0.8, 0.1, 0.1, 0.1, 0.8, 0.7, 0.2, 0.1, 0.2, 0.7, 0.1, 0.1, 0.2, 0.7},
        {6, 3}, f64());
    const Series y3 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 0.0, 1.0, 2.0}, {6}, f64());
    auto mc_or = score_metrics(probs3, y3, {MetricKind::Accuracy, MetricKind::RocAuc});
    if (!mc_or.ok()) {
      if (err) *err << "score_metrics should succeed on a valid multiclass probability matrix\n";
      return false;
    }
    const auto mc = as_f64(mc_or.value());
    if (mc.size() != 2 || !approx(mc[0], 1.0, 1e-9) || !approx(mc[1], 1.0, 1e-9)) {
      if (err) *err << "score_metrics multiclass accuracy / macro-OvR AUC should both be 1 here\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej metrics suite", "[nodej][metrics]") {
  std::ostringstream diag;
  if (!run_nodej_metrics_checks(&diag)) {
    FAIL("nodej metrics checks failed: " << diag.str());
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
