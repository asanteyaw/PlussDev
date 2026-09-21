// compose::Flow::clear_model example 2 -- resets trained() and is safe to
// call on a flow with no model.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.clear_model();  // no model yet -- a no-op
  std::cout << "clear_model() with no model: has_model() = " << std::boolalpha << flow.has_model() << "\n";

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  const pluss::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  const cls::Status learn_status = flow.learn(batch);
  std::cout << "learn ok = " << learn_status.ok() << "\n";
  std::cout << "trained() before clear_model() = " << flow.trained() << "\n";

  flow.clear_model();
  std::cout << "trained() after clear_model() = " << flow.trained() << "\n";

  return 0;
}
