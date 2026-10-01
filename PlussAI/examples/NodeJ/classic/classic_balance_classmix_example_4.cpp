#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  std::cout << "undefined labels ok = " << std::boolalpha << cls::class_mix(pluss::series::undefined()).ok() << "\n";

  const pluss::Series empty = pluss::make_series(std::vector<double>{}, {0}, f64);
  std::cout << "empty labels ok = " << cls::class_mix(empty).ok() << "\n";

  const double nan = std::numeric_limits<double>::quiet_NaN();
  const pluss::Series with_nan = pluss::make_series({0.0, 1.0, nan}, {3}, f64);
  std::cout << "non-finite label ok = " << cls::class_mix(with_nan).ok() << "\n";

  return 0;
}
