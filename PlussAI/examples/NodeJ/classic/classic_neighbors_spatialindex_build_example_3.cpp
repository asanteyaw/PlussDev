#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 5.0, 10.0, 15.0}, {4}, f64).reshape({4, 1});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();
  std::cout << "size() = " << index.size() << ", dim() = " << index.dim() << "\n";

  return 0;
}
