#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  flow.learn(batch);
  std::cout << "trained() after learn        = " << std::boolalpha << flow.trained() << "\n";

  flow.mark_trained(false);
  std::cout << "trained() after mark_trained = " << flow.trained() << "\n";
  std::cout << "infer ok now (blocked)        = " << flow.infer(batch.features).ok() << "\n";
  std::cout << "map ok now (unaffected)       = " << flow.map(batch.features).ok() << "\n";

  return 0;
}
