#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow with_model = made.move_value();
  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  with_model.learn(batch);
  std::cout << "with model:    dumped tensors = " << with_model.dump_model_state().value().size() << "\n";

  cls::Flow transform_only;
  transform_only.add_standard_scaler();
  transform_only.learn_map(batch.features);
  std::cout << "transform-only: dumped tensors = " << transform_only.dump_model_state().value().size() << "\n";

  return 0;
}
