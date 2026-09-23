#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 4.0, 4.0, 5.0, 5.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::LeastSquares;
  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts};

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 3, 2).value();
  std::cout << "grid_a (3 points) = " << surface.grid_a << "\n";
  std::cout << "grid_b (2 points) = " << surface.grid_b << "\n";
  std::cout << "mean_prediction shape = [" << surface.mean_prediction.sizes()[0] << ", " << surface.mean_prediction.sizes()[1] << "]\n";

  return 0;
}
