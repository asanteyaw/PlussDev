#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// selection::make_scorer builds a custom objective from a callable evaluated on
// each fold's (truth, prediction). Here: cost-weighted accuracy -- a false
// negative costs 5x a false positive -- a metric with no MetricKind. compare()
// ranks the candidates by it exactly as it would a built-in.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace selection = lymph::selection;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const Series x1 = pluss::make_series(
      {2., 1., 4., 3., 6., 5., 8., 7., 9., 11., 10., 13., 12., 15., 14., 16.}, {16}, f64);
  const Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const auto cost_weighted_accuracy = [](const Series& truth, const Series& prediction) -> double {
    const Series t = truth.reshape({truth.numel()}).to(ScalarType::Float64);
    const Series p = prediction.reshape({prediction.numel()}).to(ScalarType::Float64);
    const Series is_pos = t.eq(1.0).to(ScalarType::Float64);
    const Series is_neg = t.eq(0.0).to(ScalarType::Float64);
    const double false_neg = is_pos.mul(p.eq(0.0).to(ScalarType::Float64)).sum().item<double>();
    const double false_pos = is_neg.mul(p.eq(1.0).to(ScalarType::Float64)).sum().item<double>();
    const double positives = is_pos.sum().item<double>();
    const double negatives = static_cast<double>(t.numel()) - positives;
    const double worst = 5.0 * positives + negatives;
    return worst > 0.0 ? 1.0 - (5.0 * false_neg + false_pos) / worst : 1.0;
  };

  std::vector<selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression({.fit_intercept = true}));
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const selection::CompareResult ranked = selection::compare(
      std::move(candidates), frame, data_options, cv, lymph::TrainOptions{}, lymph::AssessOptions{},
      selection::make_scorer("cost_weighted_accuracy", cost_weighted_accuracy),
      lymph::TuneObjectivePolicy::Auto, selection::FitOptions{.fit_top = 1});

  std::cout << "objective = " << ranked.report.objective_name << "\n";
  std::cout << "best      = " << ranked.report.best_name << "\n";
  for (const auto& trial : ranked.report.trials) {
    std::cout << "  " << trial.name << " -> " << trial.objective << "\n";
  }

  return 0;
}
