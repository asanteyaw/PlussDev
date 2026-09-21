#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

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

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::Random;
  tune_options.trials = 2;
  tune_options.random.deterministic = true;
  tune_options.random.seed = 3;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::TuneReport report = cls::tune(candidates, batch, tune_options).value();
  std::cout << "trial count = " << report.trials.size() << "\n";
  for (const auto& trial : report.trials) {
    const auto& opts = std::get<cls::LinearFamilyOptions>(trial.spec.options);
    std::cout << "sampled regularization_strength = " << opts.regularization_strength << "\n";
  }

  return 0;
}
