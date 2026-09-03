#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Result::best() is the winning trial's Model, already fitted on the whole
// dataset -- ready to infer. It carries the prep stages make_model attached.
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

  const lymph::selection::search::Result tuned =
      lymph::selection::grid(axes, cv).run(
          make_model, frame, data_options, lymph::TrainOptions{}, lymph::AssessOptions{},
          lymph::MetricKind::Accuracy, lymph::TuneObjectivePolicy::Maximize,
          lymph::selection::FitOptions{.fit_top = 1});

  const lymph::models::Model& best = tuned.best();
  std::cout << "best trained = " << std::boolalpha << best.trained()
            << ", stages = " << best.stage_count() << "\n";

  const pluss::Series predicted =
      best.infer(frame.features({"x1", "x2"}));
  std::cout << "predictions = " << predicted.numel() << "\n";

  return 0;
}
