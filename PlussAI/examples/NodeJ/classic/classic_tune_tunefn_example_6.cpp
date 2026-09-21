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

  const cls::ProbeSpecSampler sampler =
      [](std::int64_t trial_index, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    if (trial_index == 5) {
      return cls::StatusOr<cls::ModelSpec>(
          cls::Status::invalid_argument("sampler: refusing pool index 5."));
    }
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::TuneOptions tune_options{};
  tune_options.search = cls::SearchKind::Bayesian;
  tune_options.trials = 3;
  tune_options.split.plan = cls::CVPlanKind::CrossVal;
  tune_options.split.folds = 3;
  tune_options.split.random.seed = 0;

  const cls::StatusOr<cls::TuneReport> report = cls::tune(sampler, batch, tune_options);
  std::cout << "ok = " << std::boolalpha << report.ok() << "\n";
  std::cout << "message = \"" << report.status().message << "\"\n";

  return 0;
}
