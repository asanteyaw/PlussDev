#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// A Candidate carries an optional TrainOptions of its own. When set, that
// candidate trains under its own settings instead of compare()'s shared
// train_options -- here calibration on the logistic candidate only.
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

  lymph::TrainOptions calibrated{};
  calibrated.calibration = lymph::ScoreCalibration::Platt;

  std::vector<lymph::selection::Candidate> candidates;
  candidates.emplace_back("logreg_calibrated", lymph::models::logistic_regression(), calibrated);
  candidates.emplace_back("bayes_plain", lymph::models::gaussian_nb_classifier());

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

  std::cout << "trials = " << ranked.report.trials.size()
            << ", best = " << ranked.report.best_name << "\n\n";
  std::cout << ranked.report.board();

  return 0;
}
