<!-- generated from warm_starts_and_options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Warm starts and options

[Back to Quadratic Programming](../qp_01_quadratic_programs.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
const double inf = std::numeric_limits<double>::infinity();
Series mat(std::vector<std::vector<double>> rows) {
  std::vector<double> flat;
  for (const auto& r : rows) flat.insert(flat.end(), r.begin(), r.end());
  return pluss::make_series(flat, {static_cast<std::int64_t>(rows.size()), static_cast<std::int64_t>(rows[0].size())}).to(f64);
}
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
  const auto a = mat({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
  const auto lower = vec({1.0, 0.0, 0.0}), upper = vec({1.0, 0.7, 0.7});

  const auto first = qp::solve(p, vec({1.0, 1.0}), a, lower, upper);
  std::cout << "first solve: " << first.iterations << " iterations\n";
  const auto q2 = vec({1.05, 0.95});  // a slightly different problem
  const auto cold = qp::solve(p, q2, a, lower, upper);
  const auto warm = qp::solve(p, q2, a, lower, upper, qp::Options{}, first.warm());
  std::cout << "changed q, cold start: " << cold.iterations << " iterations\n";
  std::cout << "changed q, warm start: " << warm.iterations << " iterations\n\n";

  std::cout << std::left << std::setw(30) << "options" << std::setw(12) << "iterations" << std::setw(12) << "converged" << "x0 error\n";
  auto run = [&](const char* label, qp::Options o) {
    const auto r = qp::solve(p, vec({1.0, 1.0}), a, lower, upper, o);
    std::cout << std::left << std::setw(30) << label << std::setw(12) << r.iterations << std::setw(12) << (r.converged ? "yes" : "no")
              << std::scientific << std::setprecision(1) << std::abs(r.x.to_vec<double>()[0] - 0.3) << "\n";
  };
  run("defaults (1e-5)", qp::Options{});
  qp::Options tight; tight.abs_tol = 1e-9; tight.rel_tol = 1e-9;
  run("tolerances 1e-9", tight);
  qp::Options loose; loose.abs_tol = 1e-3; loose.rel_tol = 1e-3;
  run("tolerances 1e-3", loose);
  qp::Options capped; capped.max_iters = 20;
  run("max_iters 20", capped);
  qp::Options bigrho; bigrho.rho = 5.0;
  run("rho 5", bigrho);
  qp::Options smallrho; smallrho.rho = 0.005;
  run("rho 0.005", smallrho);
  qp::Options fixedrho; fixedrho.adaptive_rho = false;
  run("adaptive_rho off", fixedrho);
  qp::Options fixedsmall; fixedsmall.adaptive_rho = false; fixedsmall.rho = 0.005;
  run("adaptive_rho off, rho 0.005", fixedsmall);
}
```

Output

```text
first solve: 80 iterations
changed q, cold start: 80 iterations
changed q, warm start: 50 iterations

options                       iterations  converged   x0 error
defaults (1e-5)               80          yes         3.4e-05
tolerances 1e-9               160         yes         3.0e-09
tolerances 1e-3               40          yes         3.7e-03
max_iters 20                  20          no          3.8e-02
rho 5                         20          yes         8.1e-06
rho 0.005                     100         yes         2.3e-05
adaptive_rho off              350         yes         5.1e-05
adaptive_rho off, rho 0.005   4000        no          1.5e-03
```
- **Warm start.** A slightly changed problem restarted from the previous `warm()` takes fewer iterations than a cold start (50 against 80 here). The benefit grows with how close the new problem is, which is the situation in model predictive control.
- **Tolerances** trade iterations for accuracy: 1e-9 doubles the work and gains four digits; 1e-3 halves it and loses two.
- **Penalty.** The starting `rho` hardly matters, because the penalty is rebalanced during the run: starts spanning three orders of magnitude (0.005, 0.1, 5) all converge within about a hundred iterations. Switch `adaptive_rho` off and the starting value matters a great deal; a poor one needs several times the iterations, or fails within the cap.
- **Iteration cap.** Stopping at the cap returns the current iterate with `converged` false. Check it.
