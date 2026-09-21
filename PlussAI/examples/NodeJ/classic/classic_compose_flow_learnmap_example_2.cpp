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

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});

  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "has_model()          = " << std::boolalpha << flow.has_model() << "\n";
  std::cout << "learn_map ok         = " << mapped.ok() << "\n";
  std::cout << "trained() after      = " << flow.trained() << "  (model never fit)\n";

  return 0;
}
