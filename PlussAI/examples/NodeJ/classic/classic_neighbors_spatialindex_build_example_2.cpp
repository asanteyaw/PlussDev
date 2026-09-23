#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::Series rows = pluss::make_series({0.0, 0.0, 10.0, 10.0}, {4}, f64).reshape({2, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  // Reassign the local variable entirely -- the index was already built
  // from the original data and cannot see this change.
  rows = pluss::make_series({999.0, 999.0, 999.0, 999.0}, {4}, f64).reshape({2, 2});

  const pluss::Series query = pluss::make_series({0.1, 0.1}, {2}, f64).reshape({1, 2});
  const cls::KNearestResult result = index.query_k(query, 1).value();
  std::cout << "nearest index (still 0, the original first point) = " << result.indices << "\n";

  return 0;
}
