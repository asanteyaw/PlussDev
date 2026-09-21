#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_standard_scaler();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
