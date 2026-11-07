// as_dtype(double, ScalarType, Config) builds a one-element series from a host double, the same job
// as_f32(value, cfg) does for a chosen type, but the type is a value here, not part of the name.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto value = pluss::tils::as_dtype(2.5, pluss::ScalarType::Float32, cfg);
  std::cout << "dtype: " << value.dtype() << '\n';
  std::cout << "holds one element: " << (value.numel() == 1) << '\n';
  return 0;
}
