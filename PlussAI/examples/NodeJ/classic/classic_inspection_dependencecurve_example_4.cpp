#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 1 is exactly 7.0 for every row.
  const cls::Series x = pluss::make_series(
      {0.0, 7.0, 0.1, 7.0, 10.0, 7.0, 10.1, 7.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve curve = cls::dependence_curve(spec, batch, batch.features, 1, 4).value();
  std::cout << "grid (constant feature, every point = 7.0) = " << curve.grid << "\n";

  return 0;
}
