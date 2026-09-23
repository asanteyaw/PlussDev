#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y*2 (informative); feature 1 is unrelated noise.
  const cls::Series x = pluss::make_series(
      {2.0, 5.0, 4.0, 1.0, 6.0, 9.0, 8.0, 4.0, 10.0, 2.0, 12.0, 8.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::LeastSquares;
  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts};

  cls::RandomState random{};
  random.seed = 2;
  const cls::PermuteImpactReport report = cls::permutation_impact(spec, batch, batch, cls::MetricKind::Mse, 5, random).value();
  std::cout << "mean_drop (feature 0 informative, feature 1 noise) = " << report.mean_drop << "\n";

  return 0;
}
