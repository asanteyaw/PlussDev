// Infinite limits. Pass +-infinity as a limit and `quadra` rewrites the integral on a finite interval by a change of variables, then
// integrates that; no other change is needed. Three integrals: the Gaussian integral over the whole line (sqrt(pi) = 1.7724538509055159),
// e^{-x} over [0, infinity) (exactly 1), and a complex-valued one, the integral of e^{-x} e^{ix} over [0, infinity) = 1/(1 - i)
// = 0.5 + 0.5i. The values are accurate to the working tolerance, not to machine precision, because the far tails are cut at
// `infinite_domain_epsilon`.
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
  const double inf = std::numeric_limits<double>::infinity();
  std::cout << std::setprecision(12);
  {
    integ::IntegrandFn g = [](const Series& x) { return (x * x * -1.0).exp(); };
    const auto r = integ::quadra(g, -inf, inf);
    std::cout << "gaussian, (-inf, inf)  : " << r.value.item<double>() << "  (exact " << std::sqrt(pi) << ")  converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn e = [](const Series& x) { return (x * -1.0).exp(); };
    const auto r = integ::quadra(e, 0.0, inf);
    std::cout << "exp(-x), [0, inf)      : " << r.value.item<double>() << "  (exact 1)  converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn c = [](const Series& x) {
      const Series damp = (x * -1.0).exp();
      return pluss::series::complex(damp * x.cos(), damp * x.sin());
    };
    const auto r = integ::quadra(c, 0.0, inf);
    const auto v = r.value.item<std::complex<double>>();
    std::cout << "exp(-x) e^{ix}, [0,inf): " << v.real() << " + " << v.imag() << "i  (exact 0.5 + 0.5i)\n";
  }
}
