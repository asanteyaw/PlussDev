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

  const cls::DependenceSurface surface = cls::dependence_surface(spec, batch, batch.features, 0, 1, 4, 2).value();
  std::cout << "mean_prediction shape = [" << surface.mean_prediction.sizes()[0] << ", " << surface.mean_prediction.sizes()[1] << "]\n";

  return 0;
}
