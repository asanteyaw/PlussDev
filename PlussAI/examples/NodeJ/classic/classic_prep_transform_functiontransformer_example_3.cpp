#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x); });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  transformer.learn(batch);

  const cls::StatusOr<std::vector<pluss::Series>> state = transformer.dump_state();
  std::cout << "ok = " << std::boolalpha << state.ok() << "\n";
  std::cout << "message = \"" << state.status().message << "\"\n";

  return 0;
}
