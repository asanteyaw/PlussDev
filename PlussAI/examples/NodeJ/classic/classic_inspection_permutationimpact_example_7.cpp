#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 10.0, 2.0, 10.1, 8.0, 0.1, 1.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions opts{};
  opts.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts};

  std::cout << "repeats=0 ok = " << std::boolalpha << cls::permutation_impact(spec, batch, batch, cls::MetricKind::Accuracy, 0).ok() << "\n";

  cls::LabeledBatch mismatched{};
  mismatched.features.x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  mismatched.y = pluss::make_series({0.0, 1.0, 1.0, 0.0}, {4}, f64);
  std::cout << "train/eval feature-count mismatch ok = " << cls::permutation_impact(spec, batch, mismatched).ok() << "\n";

  return 0;
}
