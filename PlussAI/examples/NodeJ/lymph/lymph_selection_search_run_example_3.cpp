#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A two-axis grid, run through the (x, y) data shape, with FitOptions.fit_top
// set high enough that Result::top(k) can hand back the k best trials as owned
// (name, Model) pairs.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x1 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x2 = pluss::make_series(
      {800., 3200., 900., 3000., 1100., 3400., 750., 3100.,
       850., 3300., 1000., 2900., 950., 3500., 1050., 3050.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, {16}, f64);
  const DataFrame x({{"x1", pluss::table::Slate(x1)}, {"x2", pluss::table::Slate(x2)}});

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  const auto make_model = [](const lymph::selection::GridPoint& point) {
    lymph::models::Model m = lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
    m.standard_scaler();
    return m;
  };

  lymph::selection::CrossValOptions cv{};
  cv.folds = 4;
  cv.shuffle = false;

  const lymph::selection::Search search = lymph::selection::grid(axes, cv);
  const lymph::selection::search::Result tuned =
      search.run(make_model, x, y, lymph::TrainOptions{}, lymph::AssessOptions{},
                 lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
                 lymph::selection::FitOptions{.fit_top = 3});

  std::cout << "grid points = " << tuned.report.trials.size() << "\n";
  const auto top2 = tuned.top(2);
  std::cout << "top(2):\n";
  for (const auto& [name, model] : top2) {
    std::cout << "  " << name << "  trained=" << std::boolalpha << model.trained() << "\n";
  }

  return 0;
}
