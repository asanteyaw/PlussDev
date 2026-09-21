#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

namespace {
std::vector<pluss::ai::classic::ModelSpec> regularization_candidates() {
  namespace cls = pluss::ai::classic;
  std::vector<cls::ModelSpec> out{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    out.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }
  return out;
}
}  // namespace

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.9, 2.0, 2.1, 2.2}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::TuneOptions tune_options{};
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const std::vector<cls::ModelSpec> candidates = regularization_candidates();
  const cls::TuneReport via_tune = cls::tune(candidates, batch, tune_options).value();
  const cls::TuneReport via_sweep = cls::sweep(candidates, batch, tune_options.split).value();

  std::cout << "best_index matches = " << std::boolalpha << (via_tune.best_index == via_sweep.best_index) << "\n";
  std::cout << "trial count matches = " << (via_tune.trials.size() == via_sweep.trials.size()) << "\n";

  return 0;
}
