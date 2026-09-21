#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::StandardScaler fresh;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  const cls::StatusOr<pluss::Series> too_early = fresh.map(probe);
  std::cout << "map before learn: ok = " << std::boolalpha << too_early.ok()
            << ", message = \"" << too_early.status().message << "\"\n";

  cls::FeatureBatch train{};
  train.x = pluss::make_series({-1.0, 9.0, 1.0, 11.0}, {4}, f64).reshape({2, 2});
  cls::prep::StandardScaler original;
  original.learn(train);
  cls::FeatureBatch minus_one_sigma{};
  minus_one_sigma.x = pluss::make_series({-1.0, 9.0}, {2}, f64).reshape({1, 2});  // == row 0, exactly 1 stddev below each mean
  const pluss::Series original_out = original.map(minus_one_sigma).value();

  const std::vector<pluss::Series> state = original.dump_state().value();
  cls::prep::StandardScaler reloaded;
  std::cout << "trained() before load_state = " << std::boolalpha << reloaded.trained() << "\n";
  reloaded.load_state(state);
  std::cout << "trained() after load_state  = " << reloaded.trained() << "\n";

  const pluss::Series reloaded_out = reloaded.map(minus_one_sigma).value();
  std::cout << "original.map(-1 stddev row) = " << original_out << "\n";
  std::cout << "reloaded.map(-1 stddev row) = " << reloaded_out << "\n";

  return 0;
}
