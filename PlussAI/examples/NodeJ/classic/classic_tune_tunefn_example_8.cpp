#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::ProbeSpecSampler sampler =
      [](std::int64_t, std::uint64_t) -> cls::StatusOr<cls::ModelSpec> {
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  };

  cls::TuneOptions zero_trials{};
  zero_trials.search = cls::SearchKind::Grid;
  zero_trials.trials = 0;
  const cls::StatusOr<cls::TuneReport> zero_report = cls::tune(sampler, batch, zero_trials);
  std::cout << "zero-trials message = \"" << zero_report.status().message << "\"\n";

  cls::TuneOptions with_trials{};
  with_trials.search = cls::SearchKind::Grid;
  with_trials.trials = 3;
  const cls::StatusOr<cls::TuneReport> grid_report = cls::tune(sampler, batch, with_trials);
  std::cout << "grid message = \"" << grid_report.status().message << "\"\n";

  return 0;
}
