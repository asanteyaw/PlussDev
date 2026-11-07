// to_scalar is the general form of the extraction. It names the type you want, and a trait tells you
// which types can be extracted at all. Here the program checks the traits before it asks.
#include <pluss/mic.h>
#include <pluss/tils/kit/scalar_cast.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto total = pluss::make_series(std::vector<std::int64_t>{128}, {}, cfg);

  std::cout << "int64 can be extracted: " << pluss::tils::is_scalar_extractable_v<std::int64_t> << '\n';
  std::cout << "std::string can be extracted: " << pluss::tils::is_scalar_extractable_v<std::string> << '\n';
  std::cout << "total: " << pluss::tils::to_scalar<std::int64_t>(total) << '\n';
  return 0;
}
