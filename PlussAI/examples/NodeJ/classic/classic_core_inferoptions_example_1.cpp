// core::InferOptions example 1 -- the same fitted classifier, three
// positive_threshold values.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  // A probe row right at the model's decision boundary.
  const cls::Series probe = pluss::make_series({3.0}, {1}, f64).reshape({1, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = probe;

  const double score = made.value()->infer_scores(probe_batch).value().item<double>();
  std::cout << "raw score = " << score << "\n";

  for (double threshold : {0.2, 0.5, 0.8}) {
    cls::InferOptions opts{};
    opts.positive_threshold = threshold;
    const double predicted = made.value()->infer(probe_batch, opts).value().item<double>();
    std::cout << "positive_threshold = " << threshold << " -> predicted class = " << predicted << "\n";
  }

  return 0;
}
