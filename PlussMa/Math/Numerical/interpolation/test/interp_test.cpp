#include "pluss/ma.h"

#include <cmath>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

namespace {
const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int failures = 0;
void expect(bool ok, const std::string &what) {
  if (!ok) {
    ++failures;
    std::cerr << "[FAIL] " << what << '\n';
  }
}
bool near(double a, double b, double tol = 1e-12) { return std::abs(a - b) <= tol; }
} // namespace

int main() {
  const Series x = vec({0.0, 1.0, 3.0});
  const Series y = vec({0.0, 2.0, 2.0});

  const auto inside = interp::linear(x, y, vec({0.5, 2.0})).host<double>();
  expect(near(inside[0], 1.0) && near(inside[1], 2.0), "linear interpolation at interior points");
  expect(near(interp::linear(x, y, pluss::make_series(0.5).to(f64)).item<double>(), 1.0), "scalar query gives a scalar");

  const auto clamp = interp::linear(x, y, vec({-1.0, 5.0}), interp::EdgeMode::Clamp).host<double>();
  expect(near(clamp[0], 0.0) && near(clamp[1], 2.0), "Clamp holds the end values");
  const auto line = interp::linear(x, y, vec({-1.0, 5.0}), interp::EdgeMode::Linear).host<double>();
  expect(near(line[0], -2.0) && near(line[1], 2.0), "Linear extends the end segments");
  bool threw = false;
  try {
    (void)interp::linear(x, y, vec({5.0}), interp::EdgeMode::Error);
  } catch (const std::out_of_range &) {
    threw = true;
  }
  expect(threw, "Error mode throws outside the table");

  // batches share the coordinate
  const Series curves = pluss::make_series(std::vector<double>{0, 2, 2, 0, 1, 2}, {2, 3}).to(f64);
  const auto batch = interp::linear(x, curves, vec({0.5})).host<double>();
  expect(batch.size() == 2 && near(batch[0], 1.0) && near(batch[1], 0.5), "batch of curves");

  // complex values are interpolated componentwise (they used to fail the finiteness check)
  const Series z = pluss::make_series(std::vector<std::complex<double>>{{0, 0}, {2, 4}, {2, 4}}, {3}).to(pluss::ScalarType::Complex128);
  const auto c = interp::linear(x, z, pluss::make_series(0.5).to(f64)).item<std::complex<double>>();
  expect(near(c.real(), 1.0) && near(c.imag(), 2.0), "complex values interpolate");

  // gradient to the values: weights of the bracketing samples
  Series tracked = vec({0.0, 2.0, 2.0});
  tracked.tracks_slope_(true);
  interp::linear(x, tracked, pluss::make_series(0.25).to(f64)).reverse();
  const auto g = tracked.sensitivity().host<double>();
  expect(near(g[0], 0.75) && near(g[1], 0.25) && near(g[2], 0.0), "gradient equals the interpolation weights");

  if (failures != 0) return 1;
  std::cout << "math interp checks passed\n";
  return 0;
}
