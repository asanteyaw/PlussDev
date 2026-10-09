#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// CompareResult wraps the report with fitted models. best() is the winning
// candidate re-fitted on the whole dataset -- trained, ready to infer.
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
                                lymph::selection::FitOptions{.fit_top = 1});

  const lymph::models::Model& winner = ranked.best();
  std::cout << "winner = " << ranked.report.best_name
            << ", trained = " << std::boolalpha << winner.trained() << "\n";
  const pluss::Series predicted = winner.infer(frame.features({"x0", "x1"}));
  std::cout << "predictions = " << predicted.numel() << "\n";

  return 0;
}
