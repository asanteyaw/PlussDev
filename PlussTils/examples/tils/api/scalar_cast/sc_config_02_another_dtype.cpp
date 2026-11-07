// A second dtype, to show the config form is uniform across the family: every as_<dtype>(Config)
// sets the same field on the configuration, whichever type you pick.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::tils::as_bool(pluss::Config{}.with_platform(pluss::pCPU));
  const auto flags = pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, cfg);
  std::cout << "dtype: " << flags.dtype() << '\n';
  return 0;
}
