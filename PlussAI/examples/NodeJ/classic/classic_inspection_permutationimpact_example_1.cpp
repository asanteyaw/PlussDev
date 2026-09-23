#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 separates the classes cleanly; feature 1 is unrelated noise.
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
  random.seed = 3;
  const cls::PermuteImpactReport report = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 5, random).value();
  std::cout << "mean_drop = " << report.mean_drop << "\n";
  std::cout << "std_drop  = " << report.std_drop << "\n";

  return 0;
}
