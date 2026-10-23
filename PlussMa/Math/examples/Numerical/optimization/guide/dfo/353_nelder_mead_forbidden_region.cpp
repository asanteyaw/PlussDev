// Forbidden regions. An objective may return +infinity to say "this point is not allowed"; the simplex treats it as a
// very bad value and moves away. This is a cheap way to keep a search inside a region that is easy to test but awkward
// to write as a bound, here the disc x0^2 + x1^2 <= 4. Minimise (x0-3)^2 + (x1-3)^2, whose unconstrained minimum is
// outside the disc: the answer is the nearest point of the disc, (sqrt 2, sqrt 2) = (1.4142, 1.4142), value 5.0294.
// NaN and -infinity are errors, because they cannot be ordered.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto f = [](const pluss::Series& x) -> double {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a * a + b * b > 4.0) return std::numeric_limits<double>::infinity();
    return (a - 3) * (a - 3) + (b - 3) * (b - 3);
  };
  const auto o = dfo::nelder_mead_minimizer(f, vec({0.0, 0.0}));
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << o.minimizer.select(0, 0).item<double>() << ", "
            << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "minimum   : " << o.minimum << "  (exact 5.0294)\n";
  std::cout << "evaluations: " << o.diagnostics.evaluations << "\n";

  auto bad = [](const pluss::Series&) -> double { return std::nan(""); };
  try {
    (void)dfo::nelder_mead_minimizer(bad, vec({0.0, 0.0}));
  } catch (const std::exception&) {
    std::cout << "a NaN objective is rejected with an error\n";
  }
}
