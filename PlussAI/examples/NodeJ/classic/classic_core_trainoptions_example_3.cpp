// core::TrainOptions example 3 -- max_iterations controls how far an
// iterative solver gets. Ordinary least squares is a closed-form solve
// (max_iterations does not apply to it), so this uses Logistic, which
// genuinely iterates: a 1-iteration fit and a 2-iteration fit land on
// visibly different raw scores for the same probe row.

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
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({4.5}, {1}, f64).reshape({1, 1});

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;

  auto fit_and_score = [&](std::int64_t max_iterations) {
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
        cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
    cls::TrainOptions opts{};
    opts.max_iterations = max_iterations;
    made.value()->learn(batch, opts);
    return made.value()->infer_scores(probe).value().item<double>();
  };

  std::cout << "score after max_iterations = 1 : " << fit_and_score(1) << "\n";
  std::cout << "score after max_iterations = 2 : " << fit_and_score(2) << "\n";
  std::cout << "score after max_iterations = 1000 (default) : " << fit_and_score(1000) << "\n";

  return 0;
}
