#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A Search built from grid(axes, cv) stores a CV split policy, but run(...)
// accepts an explicit SplitOptions to override it for one call -- here a single
// 30% holdout instead of k folds.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame frame({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x1", "x2"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
  };
  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = true,
    });
    m.standard_scaler();
    return m;
  };

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = false;
  const lymph::selection::Search search = lymph::selection::grid(axes, cv);

  // One-call override: plain holdout, 30% test, fixed seed.
  lymph::SplitOptions holdout{};
  holdout.plan = lymph::CVPlanKind::Holdout;
  holdout.holdout_fraction = 0.3;
  holdout.shuffle = false;

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 200;

  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, holdout, train_options,
                 lymph::AssessOptions{}, lymph::MetricKind::Accuracy,
                 lymph::TuneObjectivePolicy::Maximize, lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name << "\n\n";
  std::cout << tuned.report.board();

  return 0;
}
