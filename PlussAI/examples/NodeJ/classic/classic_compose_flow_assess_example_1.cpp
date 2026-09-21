#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::LabeledBatch train{};
  train.features.x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  train.y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  flow.learn(train);

  cls::LabeledBatch held_out{};
  held_out.features.x = pluss::make_series({1.1, 2.5}, {2}, f64).reshape({2, 1});
  held_out.y = pluss::make_series({0.0, 1.0}, {2}, f64);

  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Accuracy};
  const cls::StatusOr<pluss::Series> scored = flow.assess(held_out, opts);
  std::cout << "ok    = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "score = " << scored.value() << "\n";

  return 0;
}
