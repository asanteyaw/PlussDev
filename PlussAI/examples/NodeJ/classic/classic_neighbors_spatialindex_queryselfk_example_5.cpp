#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 0.0}, {2}, f64).reshape({1, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  const cls::StatusOr<cls::KNearestResult> rejected = index.query_self_k(1);
  std::cout << "ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
