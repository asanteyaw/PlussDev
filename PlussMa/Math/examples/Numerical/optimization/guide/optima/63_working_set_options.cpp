// The settings that control how the active list changes. `add_constraint_tolerance` is how close to its limit a
// constraint must come before it is added; `drop_multiplier_tolerance` is how small a multiplier must be before
// the constraint is dropped; `favor_working_set_updates` lets the list change between steps instead of waiting for
// a full solve; `max_working_set_violations` caps how many constraints may be added in one iteration. The
// default problem is the one of the previous program with 500 extra rows.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// min sum (x_j - 2)^4 + 0.1 x_j^2 over 6 variables (not quadratic, so every iteration is a genuine step), with `extra` linear inequalities on top of two that matter:
// sum x <= 6 and x_0 <= 0.5. The extra rows have generous right-hand sides and are never active.
struct Rows { pluss::Series A, b; };
Rows make_rows(int extra) {
  const int n = 6, m = 2 + extra;
  std::vector<double> A(static_cast<std::size_t>(m * n), 0.0), b(static_cast<std::size_t>(m), 0.0);
  for (int j = 0; j < n; ++j) A[j] = 1.0;
  b[0] = 6.0;
  A[n + 0] = 1.0;
  b[1] = 0.5;
  for (int k = 0; k < extra; ++k) {
    double l1 = 0.0;
    for (int j = 0; j < n; ++j) {
      const double a = std::sin(1.7 * (k + 1) + 0.9 * (j + 1));
      A[(2 + k) * n + j] = a;
      l1 += std::abs(a);
    }
    b[2 + k] = 3.0 * l1;
  }
  return {pluss::make_series(A, {m, n}).to(f64), pluss::make_series(b, {m}).to(f64)};
}
auto objective = [](const pluss::Series& x) { return (x - 2.0).pow(4.0).sum() + 0.1 * x.square().sum(); };

int main() {
  const auto rows = make_rows(500);
  std::cout << std::left << std::setw(46) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  auto run = [&](const std::string& label, optim::OptimaOptions o) {
    o.algorithm = optim::OptimaAlgorithm::WorkingSet;
    const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), o, optim::constraints().ln_ineq(rows.A, rows.b));
    std::cout << std::left << std::setw(46) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(6)
              << r.evaluation.value.template item<double>() << "\n";
  };
  run("defaults", {});
  for (double t : {1e-6, 1e-1, 1.0}) {
    optim::OptimaOptions o;
    o.working_set.add_constraint_tolerance = t;
    run("add_constraint_tolerance " + std::to_string(t).substr(0, 8), o);
  }
  for (double t : {1e-12, 1e-4, 1e-1}) {
    optim::OptimaOptions o;
    o.working_set.drop_multiplier_tolerance = t;
    run("drop_multiplier_tolerance " + std::to_string(t).substr(0, 8), o);
  }
  {
    optim::OptimaOptions o;
    o.working_set.favor_working_set_updates = false;
    run("favor_working_set_updates false", o);
  }
  for (std::int64_t k : {1, 2, 8}) {
    optim::OptimaOptions o;
    o.working_set.max_working_set_violations = k;
    run("max_working_set_violations " + std::to_string(k), o);
  }
  return 0;
}
