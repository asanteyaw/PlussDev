#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

// A SpecTuneResult carries a ledger of per-trial columns. has_ledger() guards
// it; objective_series / objective_rank_series / encoded_param_table pull the
// columns out as Series.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

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

  std::cout << "has_ledger = " << std::boolalpha
            << lymph::selection::has_ledger(result) << "\n";
  const pluss::Series objective = lymph::selection::objective_series(result);
  const pluss::Series rank = lymph::selection::objective_rank_series(result);
  std::cout << "objective per trial =";
  for (const double v : objective.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nrank per trial     =";
  for (const double v : rank.to_vec<double>()) std::cout << " " << v;
  std::cout << "\nencoded_param_table numel = "
            << lymph::selection::encoded_param_table(result).numel() << "\n";

  return 0;
}
