// Extracting a plain integer, not a float. The extraction form takes the type from the call, as_i64
// here, and requires the series to hold exactly one element.
#include <pluss/mic.h>
#include <pluss/tils/aux/scalar_cast.h>

#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto count = pluss::make_series(std::vector<std::int64_t>{42}, {}, cfg);
  const std::int64_t value = pluss::tils::as_i64(count);
  std::cout << "extracted: " << value << '\n';
  return 0;
}
