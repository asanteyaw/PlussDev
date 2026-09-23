#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series(
      {0.0, 0.0, 1.0, 0.0, 5.0, 5.0}, {6}, f64).reshape({3, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::KNearestResult via_query_k = index.query_k(rows, 1).value();
  std::cout << "query_k(rows, 1) indices (each row matches ITSELF)      = " << via_query_k.indices << "\n";

  const cls::KNearestResult via_self_k = index.query_self_k(1).value();
  std::cout << "query_self_k(1) indices (each row's own index excluded) = " << via_self_k.indices << "\n";

  return 0;
}
