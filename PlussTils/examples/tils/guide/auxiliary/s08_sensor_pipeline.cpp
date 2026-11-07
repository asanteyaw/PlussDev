// A small pipeline that reads raw sensor readings, rounds them to whole units for storage, and
// reports a count and an average. Each conversion is one scalar call, so the types are visible at
// each step.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto raw = pluss::make_series(std::vector<double>{21.4, 22.9, 19.2, 25.6}, {4}, cfg);

  // Round to whole degrees by adding a half and truncating, then store as integers.
  const auto shifted = raw.add(pluss::tils::as_f64(0.5, cfg));
  const auto whole = pluss::tils::as_i64(shifted, cfg);

  std::int64_t sum = 0;
  for (std::int64_t v : whole.host<std::int64_t>()) sum += v;
  const auto count = pluss::make_series(std::vector<std::int64_t>{static_cast<std::int64_t>(whole.numel())}, {}, cfg);

  std::cout << "readings: " << pluss::tils::as_i64(count) << '\n';
  std::cout << "rounded sum: " << sum << '\n';
  std::cout << "rounded average: " << static_cast<double>(sum) / static_cast<double>(whole.numel()) << '\n';
  return 0;
}
