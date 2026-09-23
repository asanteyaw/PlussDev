#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64).reshape({3, 1});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult result = index.query_self_k(100).value();
  std::cout << "requested k=100, size()-1=2 -> effective width = " << result.indices.sizes()[1] << "\n";

  return 0;
}
