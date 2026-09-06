#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

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
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

// Feature 0 is a clean linear signal for y; features 1 and 2 are pure noise,
// independent of y. Any correct importance / elimination scheme must single
// out feature 0.
LabeledBatch make_signal_noise_batch() {
  const std::vector<double> rows = {
      -4.0, 0.30, -0.10, -3.5, -0.20, 0.05, -3.0, 0.10, -0.05, -2.5, -0.40, 0.20,
      -2.0, 0.20, -0.15, -1.5, -0.10, 0.10, -1.0, 0.40, -0.20, -0.5, -0.30, 0.15,
       0.5, -0.20, 0.10,   1.0,  0.00, -0.05,  1.5,  0.10,  0.05,  2.0, -0.10, 0.20,
       2.5, -0.05, -0.10,  3.0,  0.20,  0.05,  3.5, -0.15,  0.10,  4.0,  0.05, -0.10,
       4.5, -0.10,  0.15,  5.0,  0.15, -0.05,
  };
  const Series x = pluss::make_series(rows, {18, 3}, f64());
  const Series y = pluss::make_series(
      std::vector<double>{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {18}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

bool run_feature_select_checks(std::ostream* err) {
  const LabeledBatch batch = make_signal_noise_batch();
  const models::Model model = models::logistic_regression({.fit_intercept = true});
  const models::Model trained = model.learn(batch);

  // --- permutation_importance -------------------------------------------
  const features::PermutationImportance importance = features::permutation_importance(
      trained, batch, MetricKind::Accuracy, features::PermutationOptions{.repeats = 8, .random = {.seed = 20260904ULL, .deterministic = true}});
  if (importance.importances_mean.numel() != 3 || importance.importances_std.numel() != 3 ||
      importance.importances.numel() != 3 * 8) {
    if (err) *err << "permutation_importance should return one mean/std per feature and [repeats, features] raw drops\n";
    return false;
  }
  const auto means = importance.importances_mean.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (!(means[0] > means[1]) || !(means[0] > means[2])) {
    if (err) *err << "permutation_importance should rank the informative feature above both noise features\n";
    return false;
  }
  if (importance.objective_name != "accuracy") {
    if (err) *err << "permutation_importance should record the objective name\n";
    return false;
  }

  // A custom Scorer computing the same thing as MetricKind::Accuracy should
  // agree with the built-in path.
  const auto accuracy_fn = [](const Series& truth, const Series& prediction) -> double {
    return truth.reshape({truth.numel()})
        .eq(prediction.reshape({prediction.numel()}))
        .to(ScalarType::Float64)
        .mean()
        .item<double>();
  };
  const features::PermutationImportance custom_importance = features::permutation_importance(
      trained, batch, selection::make_scorer("acc", accuracy_fn),
      features::PermutationOptions{.repeats = 8, .random = {.seed = 20260904ULL, .deterministic = true}});
  const auto custom_means = custom_importance.importances_mean.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < means.size(); ++i) {
    if (std::abs(means[i] - custom_means[i]) > 1e-9) {
      if (err) *err << "a custom accuracy Scorer should match MetricKind::Accuracy in permutation_importance\n";
      return false;
    }
  }

  bool repeats_rejected = false;
  try {
    (void)features::permutation_importance(trained, batch, MetricKind::Accuracy, features::PermutationOptions{.repeats = 0});
  } catch (const std::exception&) {
    repeats_rejected = true;
  }
  if (!repeats_rejected) {
    if (err) *err << "permutation_importance should reject PermutationOptions.repeats <= 0\n";
    return false;
  }

  // --- rfe -----------------------------------------------------------------
  const features::RFEReport report = features::rfe(
      model, batch, MetricKind::Accuracy,
      features::RFEOptions{.n_features_to_select = 1, .step = 1, .importance_repeats = 6, .random = {.seed = 20260904ULL, .deterministic = true}});
  if (report.support.size() != 3 || report.ranking.size() != 3) {
    if (err) *err << "rfe should return a support/ranking mask sized to the original feature count\n";
    return false;
  }
  if (report.selected_feature_indices.size() != 1 || report.selected_feature_indices.front() != 0) {
    if (err) *err << "rfe should keep only the informative feature (index 0)\n";
    return false;
  }
  if (!report.support[0] || report.support[1] || report.support[2]) {
    if (err) *err << "rfe support mask should match selected_feature_indices\n";
    return false;
  }
  if (report.ranking[0] != 1 || report.ranking[1] <= 1 || report.ranking[2] <= 1) {
    if (err) *err << "rfe should rank the surviving feature 1 and eliminated features higher\n";
    return false;
  }
  if (!std::isfinite(report.final_score) || report.final_score < 0.8) {
    if (err) *err << "rfe's final_score on the informative feature alone should be high\n";
    return false;
  }
  if (!report.final_model.trained()) {
    if (err) *err << "rfe should return a trained final_model\n";
    return false;
  }

  // step > 1 should still land on a valid single-feature result on this fixture.
  const features::RFEReport step_report = features::rfe(
      model, batch, MetricKind::Accuracy,
      features::RFEOptions{.n_features_to_select = 1, .step = 2, .random = {.seed = 20260904ULL, .deterministic = true}});
  if (step_report.selected_feature_indices.size() != 1 || step_report.selected_feature_indices.front() != 0) {
    if (err) *err << "rfe with step=2 should still keep the informative feature\n";
    return false;
  }

  bool n_to_select_rejected = false;
  try {
    (void)features::rfe(model, batch, MetricKind::Accuracy, features::RFEOptions{.n_features_to_select = 10});
  } catch (const std::exception&) {
    n_to_select_rejected = true;
  }
  if (!n_to_select_rejected) {
    if (err) *err << "rfe should reject n_features_to_select beyond the available feature count\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph feature select", "[lymph][features]") {
  if (!run_feature_select_checks(&std::cerr)) {
    FAIL("lymph feature select checks failed");
  }
}
#else
int main() {
  if (!run_feature_select_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph feature select checks passed\n";
  return 0;
}
#endif
