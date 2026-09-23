#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 1.0, 0.0, 5.0, 5.0, 6.0, 5.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult result = index.query_self_k(1).value();
  std::cout << "indices (each row's own index never appears: [[1],[0],[3],[2]]) = " << result.indices << "\n";

  return 0;
}
