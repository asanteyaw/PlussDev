#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// tune_specs expands a GridAxis space into ModelSpecs via a make_spec closure,
// cross-validates each, and returns a SpecTuneResult -- the reports::Tune report
// plus best_spec. Distinct from compare(), which ranks models the caller has
// already built; tune_specs builds the specs from axis definitions.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // y ~ 2*x0, so a linear spec fits it well; x1 is uninformative.
  const pluss::Series x0 = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series x1 = pluss::make_series(
      {3., 1., 4., 1., 5., 9., 2., 6., 5., 3., 5., 8., 9., 7., 9., 3.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 30., 32.}, {16}, f64);
  const DataFrame frame({{"x0", Slate(x0)}, {"x1", Slate(x1)}, {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x0", "x1"};
  data_options.label_feature = "target";

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {1e-6, 1e-3, 1e-1}},
  };
  const auto make_spec = [](const lymph::selection::GridPoint& point) {
    return lymph::ModelSpec{
        .task = lymph::TaskKind::Regression,
        .family = lymph::ModelFamily::Linear,
        .options = lymph::LinearFamilyOptions{
            .objective = lymph::LinearObjective::LeastSquares,
            .regularization = lymph::RegularizationKind::L2,
            .regularization_strength = point.at("regularization_strength"),
            .fit_intercept = true,
        },
    };
  };

  lymph::TuneOptions tune_options{};
  tune_options.search = lymph::SearchKind::Grid;
  tune_options.split.plan = lymph::CVPlanKind::CrossVal;
  tune_options.split.folds = 4;
  tune_options.split.shuffle = false;

  const lymph::selection::SpecTuneResult result =
      lymph::selection::tune_specs(axes, make_spec, frame, data_options, tune_options,
                                   lymph::TrainOptions{}, lymph::AssessOptions{},
                                   lymph::MetricKind::R2,
                                   lymph::TuneObjectivePolicy::Maximize);

  std::cout << "trials = " << result.report.trials.size()
            << ", best_index = " << result.report.best_index << "\n";
  for (const auto& trial : result.report.trials) {
    std::cout << "  strength="
              << std::get<lymph::LinearFamilyOptions>(trial.spec.options).regularization_strength
              << " objective(r2)=" << trial.objective << "\n";
  }
  std::cout << "best_spec strength = "
            << std::get<lymph::LinearFamilyOptions>(result.best_spec.options).regularization_strength
            << "\n";

  return 0;
}
