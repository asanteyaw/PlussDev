#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// random(axes, trials, random, split) samples a fixed number of points from the
// grid instead of visiting all of them -- here 3 of the 12. The RandomState
// seed makes the sample reproducible.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Slate(x1)}, {"x2", Slate(x2)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.001, 0.01, 0.1, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  std::cout << "grid_size = " << lymph::selection::grid_size(axes)
            << ", sampling 3\n";

  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
    m.standard_scaler();
    return m;
  };

  lymph::SplitOptions split{};
  split.plan = lymph::CVPlanKind::CrossVal;
  split.folds = 4;
  split.shuffle = false;

  const lymph::RandomState seed{.seed = 20260901ULL, .deterministic = true};
  const lymph::selection::Search search =
      lymph::selection::random(axes, /*trials=*/3, seed, split);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size() << "\n";
  for (const auto& trial : tuned.report.trials) {
    std::cout << "  " << trial.name << "\n";
  }

  return 0;
}
