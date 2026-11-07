// Creating a one-element Series from a host value of a chosen type. The scalar form keeps the value's
// type exact, so a count stored as int64 stays an integer on the way in and out.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <cstdint>
#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto orders = pluss::tils::as_i64(std::int64_t{1234567890123}, cfg);
  const auto threshold = pluss::tils::as_f64(0.95, cfg);

  std::cout << "orders holds one value: " << (orders.numel() == 1) << '\n';
  std::cout << "orders read back: " << pluss::tils::as_i64(orders) << '\n';
  std::cout << "threshold read back: " << pluss::tils::as_f64(threshold) << '\n';
  return 0;
}
