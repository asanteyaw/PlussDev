#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 0.1, 1.1, 0.2, 0.9, 10.0, 2.0, 10.1, 2.1, 10.2, 1.9},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  const cls::DependenceCurve coarse = cls::dependence_curve(spec, batch, batch.features, 0, 3).value();
  const cls::DependenceCurve fine = cls::dependence_curve(spec, batch, batch.features, 0, 9).value();
  std::cout << "coarse grid (3 points)  = " << coarse.grid << "\n";
  std::cout << "fine grid   (9 points)  = " << fine.grid << "\n";
  std::cout << "both endpoints match (min=0.0, max=10.2) regardless of grid_points\n";

  return 0;
}
