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

  const pluss::Series queries = pluss::make_series(
      {0.1, 0.1, 9.9, 0.1, 0.1, 9.9}, {6}, f64).reshape({3, 2});
  const cls::KNearestResult result = index.query_k(queries, 1).value();
  std::cout << "indices (expect [[0],[1],[2]]) = " << result.indices << "\n";

  return 0;
}
