// core::TrainOptions example 2 -- class_weight = Balanced on an imbalanced
// binary target.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 negatives, 1 positive, and the one positive sits right at the boundary.
  cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 5.0}, {10}, f64).reshape({10, 1});
  cls::Series y = pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, {10}, f64);

  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge

  // A probe between the two clusters -- both fits classify every training
  // row correctly regardless of weighting, so the effect only shows up as a
  // shift in the raw decision score at a point neither cluster owns outright.
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({3.0}, {1}, f64).reshape({1, 1});

  auto fit_and_score = [&](cls::ClassWeightMode mode) {
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
        cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
    cls::TrainOptions opts{};
    opts.class_weight = mode;
    made.value()->learn(batch, opts);
    return made.value()->infer_scores(probe).value().item<double>();
  };

  const double unweighted_score = fit_and_score(cls::ClassWeightMode::None);
  const double balanced_score = fit_and_score(cls::ClassWeightMode::Balanced);

  std::cout << "unweighted score at x=3.0 = " << unweighted_score << "\n";
  std::cout << "balanced score at x=3.0   = " << balanced_score
            << "  (pulled toward the minority class)\n";

  return 0;
}
