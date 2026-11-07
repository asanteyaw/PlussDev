// as_dtype(Config, ScalarType) is the generic form the named helpers (as_f64, as_i32, ...) call
// internally. Use it when the dtype is chosen at runtime, not known at the call site.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>

int main() {
  const auto base = pluss::Config{}.with_platform(pluss::pCPU);
  const pluss::ScalarType chosen = pluss::ScalarType::Int32;
  const auto cfg = pluss::tils::as_dtype(base, chosen);
  const auto values = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, cfg);
  std::cout << "dtype chosen at runtime: " << values.dtype() << '\n';
  return 0;
}
