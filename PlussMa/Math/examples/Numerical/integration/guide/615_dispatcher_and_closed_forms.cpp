// Letting the library choose, and closed forms. `integ::integra(f, a, b)` tries quadrature first and, if that misses its tolerance, takes
// whichever of quadrature and Monte Carlo reports the smaller error; the result says which one it used. `integ::analytica` holds exact
// formulas for common integrand families (gaussian, polynomial, exponential, power, sine, cosine, ...), called by name when you already
// know the form; use them to check numerical results. Here: a smooth integrand (e^x), one with a jump (a step at 0.4, which quadrature cannot resolve
// to its default tolerance of 1e-10; the dispatcher then compares quadrature's error estimate with Monte Carlo's standard error and keeps
// the smaller), and three closed forms against quadrature.
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
  std::cout << std::setprecision(10);
  {
    integ::IntegrandFn f = [](const Series& x) { return x.exp(); };
    const auto r = integ::integra(f, 0.0, 1.0);
    std::cout << "smooth : " << r.value.item<double>() << "  strategy " << (r.strategy_used == integ::IntegraStrategy::Quadrature ? "Quadrature" : "MonteCarlo")
              << ", converged " << r.converged << "\n";
  }
  {
    integ::IntegrandFn step = [](const Series& x) { return x.gt(0.4).to(f64); };
    integ::IntegraOptions o;
    o.quadrature.max_refinements = 6;
    o.monte_carlo.samples = 400000;
    o.monte_carlo.seed = 5;
    const auto r = integ::integra(step, 0.0, 1.0, o);
    std::cout << "jump   : " << r.value.item<double>() << "  (exact 0.6)  strategy " << (r.strategy_used == integ::IntegraStrategy::Quadrature ? "Quadrature" : "MonteCarlo")
              << ", error estimate " << std::scientific << std::setprecision(1) << r.error_estimate << std::fixed << std::setprecision(10) << "\n";
  }
  integ::IntegrandFn g = [](const Series& x) { return (x * x * -0.5).exp(); };
  const double q = integ::quadra(g, -1.0, 2.0).value.item<double>() / std::sqrt(2.0 * pi);
  std::cout << "gaussian P(-1 < Z < 2): analytica " << integ::analytica::gaussian_probability(-1.0, 2.0) << ", quadrature " << q << "\n";
  std::cout << "polynomial 1 + 2x + 3x^2 over [0, 2]: " << integ::analytica::polynomial({1.0, 2.0, 3.0}, 0.0, 2.0) << "  (exact 14)\n";
  std::cout << "sine(k = 2) over [0, pi/2]: " << integ::analytica::sine(2.0, 0.0, pi / 2.0) << "  (exact 1)\n";
}
