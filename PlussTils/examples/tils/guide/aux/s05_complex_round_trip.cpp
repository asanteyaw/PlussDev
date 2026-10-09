// Complex numbers keep both parts through a Series and back out. A signal's gain is a complex value,
// and the program stores it, reads it back, and checks both parts survived unchanged.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const pluss::codem::Complex128 gain{0.6, -0.8};
  const auto stored = pluss::tils::as_c128(gain);
  const auto back = pluss::tils::as_c128(stored);

  std::cout << "real part kept: " << (back.real() == 0.6) << '\n';
  std::cout << "imaginary part kept: " << (back.imag() == -0.8) << '\n';
  return 0;
}
