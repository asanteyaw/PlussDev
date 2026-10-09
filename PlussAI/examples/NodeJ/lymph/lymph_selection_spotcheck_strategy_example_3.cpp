#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// strategy(name, BalanceTarget, TrainOptions) sets both at once. Three
// strategies x two models => six trials in one run.
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
      {1., 2., 2., 3., 3., 4., 4., 5., 5., 6., 6., 7., 12., 13., 14., 15.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  lymph::models::Builder registry = lymph::models::build({
      {"bayes", lymph::models::gaussian_nb_classifier()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  lymph::BalanceTarget equalize{};
  equalize.kind = lymph::BalanceTargetKind::Equalize;

  lymph::TrainOptions calibrated{};
  calibrated.calibration = lymph::ScoreCalibration::Platt;

  lymph::selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const lymph::selection::spotcheck::Result ranked =
      lymph::selection::suite(registry)
          .strategy("plain", lymph::TrainOptions{})
          .strategy("equalized", equalize)
          .strategy("equalized_calibrated", equalize, calibrated)
          .run(frame, data_options, cv, lymph::AssessOptions{},
               lymph::MetricKind::F1, lymph::TuneObjectivePolicy::Maximize,
               lymph::selection::FitOptions{});

  std::cout << "strategies x models = " << ranked.report.trials.size() << "\n";
  for (const auto& trial : ranked.report.trials) {
    std::cout << "  " << trial.name << "\n";
  }

  return 0;
}
