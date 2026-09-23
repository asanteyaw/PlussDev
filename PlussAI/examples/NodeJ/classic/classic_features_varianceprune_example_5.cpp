#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  cls::FeatureBatch batch{};
  batch.x = x;

  std::cout << "negative floor ok  = " << std::boolalpha << cls::variance_prune(batch, -1.0).ok() << "\n";
  std::cout << "non-finite floor ok = " << cls::variance_prune(batch, std::numeric_limits<double>::infinity()).ok() << "\n";

  return 0;
}
