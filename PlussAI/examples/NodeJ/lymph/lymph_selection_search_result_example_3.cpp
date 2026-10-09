#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Result::top(k) returns the k best trials as owned (name, Model) pairs -- but
// only up to FitOptions.fit_top, since only that many models were re-fitted.
// Asking for more than were fitted throws.
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

  const lymph::selection::search::Result tuned =
      lymph::selection::grid(axes, cv).run(
          make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
          lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{.fit_top = 2});

  const auto top2 = tuned.top(2);
  std::cout << "top(2):\n";
  for (const auto& [name, model] : top2) {
    std::cout << "  " << name << "  trained=" << std::boolalpha << model.trained() << "\n";
  }

  try {
    (void)tuned.top(3);  // only 2 were fitted
  } catch (const std::exception& e) {
    std::cout << "top(3) rejected: more than fit_top were requested\n";
  }

  return 0;
}
