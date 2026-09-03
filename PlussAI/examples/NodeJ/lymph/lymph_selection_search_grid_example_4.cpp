#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// build(axes, TuneOptions) is the low-level factory: it takes a whole
// TuneOptions (search kind, trial count, seed, split, worker count) instead of
// separate arguments. Useful when the search config is already assembled as a
// value elsewhere.
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

  lymph::TuneOptions options{};
  options.search = lymph::SearchKind::Grid;
  options.max_parallel = 1;
  options.split.plan = lymph::CVPlanKind::CrossVal;
  options.split.folds = 4;
  options.split.shuffle = false;

  const lymph::selection::Search search = lymph::selection::build(axes, options);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{});

  std::cout << "trials run = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name << "\n";

  return 0;
}
