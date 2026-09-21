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

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  batch.y = pluss::make_series({1.0}, {1}, f64);
  const cls::StatusOr<pluss::Series> scored = flow.assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "message = \"" << scored.status().message << "\"\n";

  return 0;
}
