#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Search::run calls make_model(GridPoint) at every point in the grid, cross-
// validates that full Model (prep stages included), and ranks the trials by an
// objective metric. grid(axes, cv) makes the search exhaustive under a
// cross-validation split policy.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Two predictors on very different scales -- only separable if the attached
  // standard_scaler in make_model actually runs per trial.
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

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 200;
  train_options.tolerance = 1e-6;

  const lymph::selection::Search search = lymph::selection::grid(axes, cv);
  const lymph::selection::search::Result tuned =
      search.run(make_model, frame, data_options, train_options, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{.fit_top = 1});

  std::cout << "trials = " << tuned.report.trials.size()
            << ", best = " << tuned.report.best_name
            << ", best trained = " << std::boolalpha << tuned.best().trained()
            << ", best stages = " << tuned.best().stage_count() << "\n\n";
  std::cout << tuned.report.board();

  return 0;
}
