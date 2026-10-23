// The rules. Trapezoidal is exact for straight lines, Simpson for cubics, and Gauss-Legendre (panels of a fixed-order rule) reaches
// near machine precision on smooth functions with very few nodes. By default the rule is refined (the node count doubled) until its
// error estimate falls below `tolerance`. Three integrands over [0, 1]: smooth e^x, oscillatory sin(20 x), and a kink |x - 0.3|.
// Columns: the rule, whether it converged, the evaluations used and the error of the value against the exact integral.
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
  struct Case { const char* name; integ::IntegrandFn f; double exact; };
  const Case cases[] = {
      {"e^x", [](const Series& x) { return x.exp(); }, std::exp(1.0) - 1.0},
      {"sin(20 x)", [](const Series& x) { return (x * 20.0).sin(); }, (1.0 - std::cos(20.0)) / 20.0},
      {"|x - 0.3|", [](const Series& x) { return (x - 0.3).abs(); }, 0.5 * 0.3 * 0.3 + 0.5 * 0.7 * 0.7},
  };
  std::cout << std::left << std::setw(14) << "integrand" << std::setw(16) << "rule" << std::setw(11) << "converged" << std::setw(13) << "evaluations" << "error\n";
  for (const auto& c : cases) {
    for (auto rule : {integ::QuadraRule::Trapezoidal, integ::QuadraRule::Simpson, integ::QuadraRule::GaussLegendre}) {
      integ::QuadraOptions o;
      o.rule = rule;
      const auto r = integ::quadra(c.f, 0.0, 1.0, o);
      std::cout << std::left << std::setw(14) << c.name << std::setw(16) << rule_name(rule) << std::setw(11) << (r.converged ? "yes" : "no")
                << std::setw(13) << r.evaluations << std::scientific << std::setprecision(1) << std::abs(r.value.item<double>() - c.exact) << std::defaultfloat << "\n";
    }
  }
}
