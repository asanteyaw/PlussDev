// Nonlinear constraints and the three policies for them. `nonlinear_constraints` returns PSConstraintValues with an
// `inequality` vector (<= 0) and/or an `equality` vector (= 0). `constraint_policy` decides how violations are handled:
//   ExtremeBarrier       a violating point is rejected outright
//   MeritPenalty         the objective plus a penalty (initial_penalty, grown by penalty_growth) on the violation
//   AugmentedLagrangian  penalty plus multiplier estimates, refined over up to `al_max_outer_iterations` outer rounds
// Problem: minimise (x0-2)^2 + (x1-2)^2 inside the unit disc. Exact optimum 3.3431 at (0.7071, 0.7071).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  struct P { const char* name; dfo::PSConstraintPolicy p; };
  const P rows[] = {{"ExtremeBarrier", dfo::PSConstraintPolicy::ExtremeBarrier},
                    {"MeritPenalty", dfo::PSConstraintPolicy::MeritPenalty},
                    {"AugmentedLagrangian", dfo::PSConstraintPolicy::AugmentedLagrangian}};
  std::cout << std::left << std::setw(22) << "policy" << std::setw(26) << "status" << std::setw(11) << "objective" << std::setw(12) << "violation"
            << std::setw(8) << "evals" << "outer rounds\n";
  for (const auto& r : rows) {
    dfo::PSConfig c;
    c.constraint_policy = r.p;
    c.nonlinear_constraints = [](const pluss::Series& x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    auto f = [](const pluss::Series& x) { return (x - 2.0).square().sum(); };
    c.max_iterations = 4000;
    c.max_evaluations = 40000;
    const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
    std::cout << std::left << std::setw(22) << r.name << std::setw(26) << dfo::to_string(o.status) << std::fixed << std::setprecision(4)
              << std::setw(11) << o.minimum << std::scientific << std::setprecision(1) << std::setw(12) << o.diagnostics.max_constraint_violation
              << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.outer_iterations << "\n";
  }
}
