#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 0.0, 4.0,
       10.0, 2.0, 10.1, 8.0, 10.2, 3.0, 10.3, 6.0},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 9;
  const cls::PermuteImpactReport few = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 3, random).value();
  const cls::PermuteImpactReport many = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 30, random).value();
  std::cout << "repeats=3  std_drop = " << few.std_drop << "\n";
  std::cout << "repeats=30 std_drop = " << many.std_drop << "\n";
  std::cout << "repeats=3  per_repeat_drops shape = [" << few.per_repeat_drops.sizes()[0] << ", " << few.per_repeat_drops.sizes()[1] << "]\n";
  std::cout << "repeats=30 per_repeat_drops shape = [" << many.per_repeat_drops.sizes()[0] << ", " << many.per_repeat_drops.sizes()[1] << "]\n";

  return 0;
}
