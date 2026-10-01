#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series rows = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64).reshape({2, 2});
  const cls::SpatialIndex index = cls::SpatialIndex::build(rows).value();

  std::cout << "undefined query_rows ok = " << std::boolalpha << index.query_k(pluss::series::undefined(), 1).ok() << "\n";

  const pluss::Series wrong_dim = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64).reshape({1, 3});
  std::cout << "dim mismatch ok = " << index.query_k(wrong_dim, 1).ok() << "\n";

  const pluss::Series ok_dim = pluss::make_series({0.5, 0.5}, {2}, f64).reshape({1, 2});
  std::cout << "k=0 ok = " << index.query_k(ok_dim, 0).ok() << "\n";

  return 0;
}
