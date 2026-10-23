// Integrating a function. `integ::quadra(f, a, b)` integrates a callable. The callable takes a whole Series of nodes and returns the
// values at all of them in one call (never one point at a time), so the integrand is ordinary array arithmetic. The result holds the
// value (a Series), an error estimate, the number of evaluations, the refinements used, whether it converged, and the rule used.
// Here: the integral of e^x over [0, 1], which is e - 1 = 1.718281828459045.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
const double pi = 3.14159265358979323846;
const char* rule_name(integ::QuadraRule r) {
  switch (r) { case integ::QuadraRule::Trapezoidal: return "Trapezoidal"; case integ::QuadraRule::Simpson: return "Simpson"; case integ::QuadraRule::GaussLegendre: return "GaussLegendre"; default: return "Auto"; }
}
int main() {
  integ::IntegrandFn f = [](const Series& x) { return x.exp(); };
  const auto r = integ::quadra(f, 0.0, 1.0);
  std::cout << std::setprecision(15) << "value       : " << r.value.item<double>() << "  (e - 1 = " << std::exp(1.0) - 1.0 << ")\n";
  std::cout << std::scientific << std::setprecision(2) << "error est.  : " << r.error_estimate << "\n";
  std::cout << "evaluations : " << r.evaluations << "\n";
  std::cout << "refinements : " << r.refinements << "\n";
  std::cout << "converged   : " << (r.converged ? "yes" : "no") << "\n";
  std::cout << "rule used   : " << rule_name(r.rule_used) << "\n";
}
