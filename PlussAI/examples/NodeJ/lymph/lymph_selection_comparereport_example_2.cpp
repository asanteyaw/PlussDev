#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// report.trials is one Trial per candidate, in candidate order (not ranked
// order). Each Trial is {name, objective, cv} -- the cross-validation report
// behind that candidate's objective value.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 7., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

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
                                lymph::selection::FitOptions{});

  for (const lymph::selection::Trial& trial : ranked.report.trials) {
    std::cout << trial.name << ": objective=" << trial.objective
              << ", folds=" << trial.cv.report.folds.size() << "\n";
  }

  return 0;
}
