#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// TuneOptions.search = SearchKind::Random draws `trials` distinct points from
// the grid instead of visiting all of them, seeded from TuneOptions.random.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {3., 1., 4., 1., 5., 9., 2., 6., 5., 3., 5., 8., 9., 7., 9., 3.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32.}, {16}, f64);
  const DataFrame frame({{"x0", Thread(x0)}, {"x1", Thread(x1)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {1e-6, 1e-4, 1e-2, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };
  std::cout << "grid_size = " << lymph::selection::grid_size(axes) << ", sampling 3\n";

  const auto make_spec = [](const lymph::selection::GridPoint& point) {
    return lymph::ModelSpec{
        .task = lymph::TaskKind::Regression,
        .family = lymph::ModelFamily::Linear,
        .options = lymph::LinearFamilyOptions{
            .regularization_strength = point.at("regularization_strength"),
            .fit_intercept = point.at("fit_intercept") != 0.0,
        },
    };
  };

  lymph::TuneOptions tune_options{};
  tune_options.search = lymph::SearchKind::Random;
  tune_options.trials = 3;
  tune_options.random = {.seed = 20260901ULL, .deterministic = true};
  tune_options.split.plan = lymph::CVPlanKind::CrossVal;
  tune_options.split.folds = 4;
  tune_options.split.shuffle = false;

  const lymph::selection::SpecTuneResult result =
      lymph::selection::tune_specs(axes, make_spec, frame, data_options, tune_options,
                                   lymph::TrainOptions{}, lymph::AssessOptions{},
                                   lymph::MetricKind::R2,
                                   lymph::TuneObjectivePolicy::Maximize);

  std::cout << "trials run = " << result.report.trials.size() << "\n";
  for (const auto& trial : result.report.trials) {
    const auto& o = std::get<lymph::LinearFamilyOptions>(trial.spec.options);
    std::cout << "  strength=" << o.regularization_strength
              << " intercept=" << std::boolalpha << o.fit_intercept << "\n";
  }

  return 0;
}
