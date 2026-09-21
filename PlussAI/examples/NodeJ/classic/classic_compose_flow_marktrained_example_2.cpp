#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_standard_scaler();
  flow.mark_trained(true);  // lying about it -- the scaler was never fit
  std::cout << "trained() = " << std::boolalpha << flow.trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> mapped = flow.map(probe);
  std::cout << "map ok = " << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
