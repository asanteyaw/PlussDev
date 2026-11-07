// Converting a whole column to a different element type. Sensor counts arrive as floats, and the
// database wants integers. The conversion truncates toward zero, so 2.9 becomes 2.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto floats = pluss::make_series(std::vector<double>{2.9, 7.0, 0.4, 11.75}, {4}, cfg);
  const auto ints = pluss::tils::as_i64(floats, cfg);

  std::cout << "integer column is int64: " << (ints.dtype().scalar == pluss::ScalarType::Int64) << '\n';
  std::cout << "values:";
  for (auto v : ints.host<std::int64_t>()) std::cout << ' ' << v;
  std::cout << '\n';
  return 0;
}
