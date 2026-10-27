// Fourth-order iteration and the derivative of a root. `roots::householder` takes one function returning the value and its first three
// derivatives together (`Derivatives{value, first, second, third}`), so a caller can share work between them. It converges in very
// few steps. Its root also carries the dependence on any parameter the function captured: when the parameter tracks slope, calling
// `reverse()` on the root gives d(root)/d(parameter) by the implicit function theorem, -(dF/dp)/(dF/dx), whatever the starting
// guess. Here the root of x^2 = p for p = 2 is sqrt 2, and d(sqrt p)/dp = 1/(2 sqrt p) = 0.35355339...
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  Series p = pluss::series::make(2.0);
  p.tracks_slope_(true);
  const auto r = roots::householder(
      [p](const Series& x) {
        return roots::Derivatives{x.square() - p, x * 2.0, pluss::series::full_like(x, 2.0), pluss::series::zeros_like(x)};
      },
      pluss::series::make(1.0), {12, 1e-13, 1e-13, 1});
  std::cout << std::setprecision(15) << "root        : " << as_f64(r.root) << "  (sqrt 2 = " << std::sqrt(2.0) << ")\n";
  std::cout << "iterations  : " << r.iterations << "\n";
  r.root.reverse();
  std::cout << "d root / dp : " << as_f64(p.sensitivity()) << "  (exact " << 0.5 / std::sqrt(2.0) << ")\n";
}
