// Tolerance, refinement and honest failure. With `adaptive` on, the node count is doubled until the error estimate is within
// `tolerance` or `max_refinements` doublings have been used; `converged` says which. A singularity in a derivative slows convergence:
// the integral of sqrt(x) over [0, 1] (exact 2/3) has an infinite slope at 0. Read `converged` and `error_estimate`, never just `value`.
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
  integ::IntegrandFn f = [](const Series& x) { return x.sqrt(); };
  std::cout << std::left << std::setw(34) << "setting" << std::setw(11) << "converged" << std::setw(13) << "evaluations" << std::setw(16) << "error estimate" << "actual error\n";
  auto run = [&](const char* label, integ::QuadraOptions o) {
    const auto r = integ::quadra(f, 0.0, 1.0, o);
    std::cout << std::left << std::setw(34) << label << std::setw(11) << (r.converged ? "yes" : "no") << std::setw(13) << r.evaluations << std::scientific
              << std::setprecision(1) << std::setw(16) << r.error_estimate << std::abs(r.value.item<double>() - 2.0 / 3.0) << std::defaultfloat << "\n";
  };
  integ::QuadraOptions simpson;
  simpson.rule = integ::QuadraRule::Simpson;
  run("Simpson, tolerance 1e-10", simpson);
  auto loose = simpson; loose.tolerance = 1e-6;
  run("Simpson, tolerance 1e-6", loose);
  auto more = simpson; more.max_refinements = 16;
  run("Simpson, 16 refinements allowed", more);
  integ::QuadraOptions gl;
  gl.rule = integ::QuadraRule::GaussLegendre;
  run("GaussLegendre, tolerance 1e-10", gl);
  integ::QuadraOptions automatic;
  automatic.rule = integ::QuadraRule::Auto;
  run("Auto (tries Gauss-Legendre first)", automatic);
  auto fixed = simpson; fixed.adaptive = false; fixed.nodes = 1025;
  run("Simpson, fixed 1025 nodes", fixed);
}
