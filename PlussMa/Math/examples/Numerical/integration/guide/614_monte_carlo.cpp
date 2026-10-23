// Monte Carlo. `aleatoria(f, a, b)` averages the integrand at random points: the error falls like 1/sqrt(samples) whatever the
// smoothness, so it is the tool for rough integrands and many dimensions, not for accuracy. The result includes the standard error.
// `stratified = true` places one point in each of `samples` equal strata, which lowers the error for free; `seed` makes a run
// repeatable. Here: the integral of sin(x) over [0, pi] (exactly 2).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <complex>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
const double pi = 3.14159265358979323846;
const char* rule_name(integ::QuadraRule r) {
  switch (r) { case integ::QuadraRule::Trapezoidal: return "Trapezoidal"; case integ::QuadraRule::Simpson: return "Simpson"; case integ::QuadraRule::GaussLegendre: return "GaussLegendre"; default: return "Auto"; }
}
int main() {
  integ::IntegrandFn f = [](const Series& x) { return x.sin(); };
  std::cout << std::left << std::setw(10) << "samples" << std::setw(24) << "plain: error / s.e." << std::setw(24) << "stratified: error / s.e." << "\n";
  for (std::int64_t n : {1000, 10000, 100000, 1000000}) {
    std::cout << std::left << std::setw(10) << n;
    for (bool strat : {false, true}) {
      integ::AleatoriaOptions o;
      o.samples = n;
      o.seed = 11;
      o.stratified = strat;
      const auto r = integ::aleatoria(f, 0.0, pi, o);
      std::ostringstream cell;
      cell << std::scientific << std::setprecision(1) << std::abs(r.value.item<double>() - 2.0) << " / " << r.standard_error;
      std::cout << std::left << std::setw(24) << cell.str();
    }
    std::cout << "\n";
  }
  integ::AleatoriaOptions same;
  same.samples = 5000; same.seed = 3;
  const double a = integ::aleatoria(f, 0.0, pi, same).value.item<double>();
  const double b = integ::aleatoria(f, 0.0, pi, same).value.item<double>();
  std::cout << "same seed twice: " << std::setprecision(12) << std::fixed << a << ", " << b << "\n";
}
