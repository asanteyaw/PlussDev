#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow transform_only;
  transform_only.add_standard_scaler();
  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  transform_only.learn_map(batch);
  const std::vector<pluss::Series> empty_model_state = transform_only.dump_model_state().value();
  std::cout << "empty dump size = " << empty_model_state.size() << "\n";

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow with_model = made.move_value();
  const cls::Status loaded = with_model.load_model_state(empty_model_state);
  std::cout << "load onto model flow ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
