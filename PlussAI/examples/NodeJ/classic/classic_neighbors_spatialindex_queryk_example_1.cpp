#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 0.0, 0.0, 10.0, 10.0, 10.0}, {8}, f64).reshape({4, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // query (1, 1): nearest is (0, 0), squared distance 1^2 + 1^2 = 2.
  const pluss::Series query = pluss::make_series({1.0, 1.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "indices (expect [[0]]) = " << result.indices << "\n";
  std::cout << "distances2 (expect [[2]]) = " << result.distances2 << "\n";

  return 0;
}
