// families::NeighborFamilyOptions example 1 -- standardize mattering when one
// feature's scale dwarfs another's.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Feature 0 is on a 0-1 scale and actually separates the classes;
  // feature 1 is on a 0-1000 scale and is pure noise. Unscaled, feature 1
  // dominates every distance.
  const cls::Series x = pluss::make_series(
      {0.1, 900.0, 0.15, 100.0, 0.85, 500.0, 0.9, 300.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](bool standardize) {
    cls::NeighborFamilyOptions opts{};
    opts.k = 1;
    opts.standardize = standardize;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Neighbor, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "standardize = false train accuracy: " << fit_and_score(false) << "\n";
  std::cout << "standardize = true  train accuracy: " << fit_and_score(true) << "\n";

  return 0;
}
