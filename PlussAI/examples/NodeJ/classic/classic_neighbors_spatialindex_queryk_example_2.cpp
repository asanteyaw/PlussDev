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

  const pluss::Series query = pluss::make_series({1.0, 1.0}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 3).value();
  std::cout << "indices    = " << result.indices << "\n";
  std::cout << "distances2 (ascending: 2, then two ties at 82) = " << result.distances2 << "\n";

  return 0;
}
