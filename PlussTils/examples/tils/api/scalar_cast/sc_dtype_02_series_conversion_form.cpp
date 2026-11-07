// as_dtype(const Series&, ScalarType, Config) converts an existing series, the same job as_i64(x, cfg)
// does, but with the target type given as a value instead of named in the function.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto floats = pluss::make_series(std::vector<double>{3.9, 1.1}, {2}, cfg);
  const auto converted = pluss::tils::as_dtype(floats, pluss::ScalarType::Int64, cfg);
  std::cout << "converted dtype: " << converted.dtype() << '\n';
  for (auto v : converted.host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
