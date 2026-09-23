#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  std::cout << "undefined ok = " << std::boolalpha << cls::SpatialIndex::build(pluss::Series::undefined()).ok() << "\n";

  const pluss::Series rank1 = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  std::cout << "rank-1 ok = " << cls::SpatialIndex::build(rank1).ok() << "\n";

  const pluss::Series zero_rows = pluss::make_series(std::vector<double>{}, {0}, f64).reshape({0, 2});
  std::cout << "zero rows ok = " << cls::SpatialIndex::build(zero_rows).ok() << "\n";

  return 0;
}
