#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A time plan is still a cross-validation: run(model, frame, data_options)
// trains and scores across the expanding-window folds and returns the same
// metrics::CrossValidationReport a k-fold plan does. Shown here on a regressor.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // A rising series with mild noise; target is next-step-ish demand.
  const pluss::Series step = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12.,
       13., 14., 15., 16., 17., 18., 19., 20.}, {20}, f64);
  const pluss::Series demand = pluss::make_series(
      {12., 13.5, 15., 16.6, 18., 19.4, 21., 22.5, 24., 25.4,
       27., 28.6, 30., 31.5, 33., 34.4, 36., 37.5, 39., 40.6}, {20}, f64);
  const DataFrame frame({{"step", Slate(step)}, {"demand", Slate(demand)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"step"};
  data_options.label_feature = "demand";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::R2, lymph::MetricKind::Mse};

  lymph::options::CrossVal cv{};
  cv.folds = 3;
  cv.time_test_size = 3;
  const lymph::selection::Plan plan = lymph::selection::time(cv);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::linear_regression(), frame, data_options, {}, assess);

  std::cout << "chronological folds = " << report.report.folds.size() << "\n\n";
  std::cout << report.board();

  return 0;
}
