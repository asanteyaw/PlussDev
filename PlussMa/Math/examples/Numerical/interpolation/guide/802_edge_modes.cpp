// Queries outside the table. `EdgeMode` says what to do beyond the first and last coordinate: Clamp (default) returns the end value,
// Linear extends the first or last segment's line, and Error throws `std::out_of_range`. Which to choose depends on what an
// out-of-range query means in your problem: a clamped value hides a mistake, an error finds it, and a line is right only when the data
// really continue that way. Here: the table y = 2x on [1, 3], read at 0 (below), 2 (inside) and 5 (above).
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const Series x = vec({1.0, 2.0, 3.0});
  const Series y = vec({2.0, 4.0, 6.0});
  const Series q = vec({0.0, 2.0, 5.0});
  std::cout << std::fixed << std::setprecision(1);
  for (auto mode : {interp::EdgeMode::Clamp, interp::EdgeMode::Linear}) {
    const auto v = interp::linear(x, y, q, mode).to_vec<double>();
    std::cout << std::left << std::setw(8) << (mode == interp::EdgeMode::Clamp ? "Clamp" : "Linear") << ": " << v[0] << ", " << v[1] << ", " << v[2] << "\n";
  }
  try {
    (void)interp::linear(x, y, q, interp::EdgeMode::Error);
  } catch (const std::out_of_range& e) {
    std::cout << "Error   : out_of_range thrown\n";
  }
  const auto inside = interp::linear(x, y, vec({1.5, 2.5}), interp::EdgeMode::Error).to_vec<double>();
  std::cout << "Error mode inside the table: " << inside[0] << ", " << inside[1] << "\n";
}
