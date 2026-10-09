#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// FitOptions.fit_top / fit_bottom control how many ranked leaders and laggards
// get re-fitted on the whole set. CompareResult then exposes best() / worst()
// and top(k) / bottom(k) as owned (name, Model) pairs.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg", lymph::models::logistic_regression());
  candidates.emplace_back("bayes", lymph::models::gaussian_nb_classifier());
  candidates.emplace_back("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::CompareResult ranked =
      lymph::selection::compare(std::move(candidates), frame, data_options, cv,
                                lymph::TrainOptions{}, lymph::AssessOptions{},
                                lymph::MetricKind::Accuracy,
                                lymph::TuneObjectivePolicy::Maximize,
                                lymph::selection::FitOptions{.fit_top = 2, .fit_bottom = 1});

  std::cout << "best = " << ranked.report.best_name
            << " (trained=" << std::boolalpha << ranked.best().trained() << ")\n";
  std::cout << "worst trained = " << ranked.worst().trained() << "\n";

  for (const auto& [name, model] : ranked.top(2)) {
    std::cout << "  top: " << name << " trained=" << model.trained() << "\n";
  }
  for (const auto& [name, model] : ranked.bottom(1)) {
    std::cout << "  bottom: " << name << " trained=" << model.trained() << "\n";
  }

  return 0;
}
