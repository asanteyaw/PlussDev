#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // rows 0 and 1 are identical; row 2 is far away.
  const pluss::Series rows = pluss::make_series(
      {3.0, 3.0, 3.0, 3.0, 100.0, 100.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult result = index.query_self_k(1).value();
  std::cout << "indices (row 0 -> 1, row 1 -> 0, row 2 -> one of the tied pair) = " << result.indices << "\n";
  std::cout << "distances2 (rows 0 and 1 each find the other at 0)          = " << result.distances2 << "\n";

  return 0;
}
