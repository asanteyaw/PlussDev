// Choosing the element type of a new Series. Each as_ function with a Config argument returns that
// configuration with the element type set, so it can be passed straight to a factory call.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto single = pluss::tils::as_f32(pluss::Config{}.with_platform(pluss::pCPU));
  const auto ints = pluss::tils::as_i32(pluss::Config{}.with_platform(pluss::pCPU));

  const auto ratios = pluss::make_series(std::vector<double>{0.25, 0.5, 0.75}, {3}, single);
  const auto counts = pluss::make_series(std::vector<double>{1, 2, 3}, {3}, ints);

  std::cout << "ratios dtype: " << ratios.dtype() << '\n';
  std::cout << "counts dtype: " << counts.dtype() << '\n';
  return 0;
}
