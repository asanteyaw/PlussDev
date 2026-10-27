// Converting down to a narrower float type. The values are the same numbers, stored in a smaller type.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto wide = pluss::make_series(std::vector<double>{1.5, 2.5}, {2}, cfg);
  const auto narrow = pluss::tils::as_f32(wide, cfg);
  std::cout << "dtype: " << narrow.dtype() << '\n';
  for (float v : narrow.host<float>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
