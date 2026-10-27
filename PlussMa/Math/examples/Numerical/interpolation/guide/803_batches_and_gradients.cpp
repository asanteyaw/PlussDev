// Many curves at once, complex values, and derivatives. `values` may have any leading batch axes, shape [..., n]: each curve shares the
// coordinate and is interpolated at the same queries, giving [..., m]. Complex values are interpolated componentwise. The result is built
// from differentiable operations, so a gradient flows back to the values. Here: three curves sampled on one grid, read at two points, then
// the derivative of the first reading with respect to the grid values: the weights of the two surrounding samples (1 - w and w).
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
  const Series x = vec({0.0, 1.0, 2.0, 3.0});
  const Series curves = pluss::make_series(std::vector<double>{0, 1, 4, 9, 0, 2, 4, 6, 1, 1, 1, 1}, {3, 4}).to(f64);
  const auto q = vec({0.5, 2.25});
  const auto r = interp::linear(x, curves, q);
  const auto v = r.host<double>();
  std::cout << "shape [" << r.size(0) << ", " << r.size(1) << "]\n";
  for (int i = 0; i < 3; ++i) std::cout << "curve " << i << ": " << v[2 * i] << ", " << v[2 * i + 1] << "\n";

  const Series z = pluss::make_series(std::vector<std::complex<double>>{{0, 0}, {1, 2}, {2, 4}, {3, 6}}, {4}).to(pluss::ScalarType::Complex128);
  const auto c = interp::linear(x, z, pluss::make_series(1.5).to(f64)).item<std::complex<double>>();
  std::cout << "complex values at 1.5: " << c.real() << " + " << c.imag() << "i\n";

  Series tracked = pluss::make_series(std::vector<double>{0, 1, 4, 9}, {4}).to(f64);
  tracked.tracks_slope_(true);
  interp::linear(x, tracked, pluss::make_series(2.25).to(f64)).reverse();
  const auto g = tracked.sensitivity().host<double>();
  std::cout << "d value(2.25) / d samples: " << g[0] << " " << g[1] << " " << g[2] << " " << g[3] << "   (weights 0.75 and 0.25 on the samples at 2 and 3)\n";
}
