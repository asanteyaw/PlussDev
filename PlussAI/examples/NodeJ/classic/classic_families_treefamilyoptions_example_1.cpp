// families::TreeFamilyOptions example 1 -- max_depth on a pattern that needs
// more than one split.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // A checkerboard: no single split separates the classes.
  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 0.1, 0.1, 0.9, 0.9,
       0.0, 1.0, 0.0, 1.0, 0.1, 0.9, 0.1, 0.9},
      {16}, f64).reshape({8, 2});
  const cls::Series y = pluss::make_series({0, 1, 1, 0, 0, 1, 1, 0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](std::int64_t depth) {
    cls::TreeFamilyOptions opts{};
    opts.max_depth = depth;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Tree, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "max_depth = 1 train accuracy : " << fit_and_score(1) << "\n";
  std::cout << "max_depth = -1 train accuracy: " << fit_and_score(-1) << "\n";

  return 0;
}
