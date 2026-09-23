#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({0.0, 1.0, 10.0, 2.0}, {4}, f64).reshape({2, 2});
  const cls::Series y = pluss::make_series({0.0, 1.0}, {2}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  std::cout << "grid_points=1 ok = " << std::boolalpha << cls::dependence_curve(spec, batch, batch.features, 0, 1).ok() << "\n";
  std::cout << "feature_index out of range ok = " << cls::dependence_curve(spec, batch, batch.features, 5, 4).ok() << "\n";

  return 0;
}
