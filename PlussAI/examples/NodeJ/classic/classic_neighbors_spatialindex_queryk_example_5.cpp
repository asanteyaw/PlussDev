#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 10.0, 10.0, 20.0, 20.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // query row 1 (10, 10) is exactly indexed point 1.
  const pluss::Series query = pluss::make_series({10.0, 10.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "indices (expect [[1]], itself)     = " << result.indices << "\n";
  std::cout << "distances2 (expect [[0]])          = " << result.distances2 << "\n";

  return 0;
}
