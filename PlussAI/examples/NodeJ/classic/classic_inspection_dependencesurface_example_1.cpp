#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 0.1, 0.1, 0.0, 0.2, 10.0, 10.0, 10.1, 10.1, 10.0, 9.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 3).value();
  std::cout << "grid_a = " << surface.grid_a << "\n";
  std::cout << "grid_b = " << surface.grid_b << "\n";
  std::cout << "mean_prediction = " << surface.mean_prediction << "\n";

  return 0;
}
