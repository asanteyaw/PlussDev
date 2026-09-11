// families::BayesFamilyOptions example 1 -- likelihood on count-shaped data.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Word-count-style features: small non-negative integers.
  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 4.0, 1.0, 0.0, 5.0, 1.0, 4.0}, {8}, f64).reshape({4, 2});
  const cls::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  auto fit_and_score = [&](cls::BayesLikelihood likelihood) {
    cls::BayesFamilyOptions opts{};
    opts.likelihood = likelihood;
    cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Bayes, opts});
    made.value()->learn(batch);
    cls::AssessOptions assess{};
    assess.metrics = {cls::MetricKind::Accuracy};
    return made.value()->assess(batch, assess).value().item<double>();
  };

  std::cout << "Gaussian train accuracy:     " << fit_and_score(cls::BayesLikelihood::Gaussian) << "\n";
  std::cout << "Multinomial train accuracy:  " << fit_and_score(cls::BayesLikelihood::Multinomial) << "\n";

  return 0;
}
