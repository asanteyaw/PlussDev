<!-- generated from quadratic_programs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Quadratic programs

[Back to Quadratic Programming](../qp_01_quadratic_programs.md)

A quadratic program minimises a convex quadratic `0.5 x'Px + q'x` subject to linear constraints `l <= Ax <= u`. It is the problem behind constrained least squares, portfolio selection and model predictive control, and the solver is a first-order method that factorises the problem once and then iterates, with an adaptive penalty.

The page shows the solver on its own terms. A first program solves a small QP and reads the multipliers (an active constraint has a large multiplier, a slack bound has 0). The constraint forms (equality, one-sided bound, unconstrained) use the same call and differ only in the rows and bounds. Batches and reuse are the efficient route when only `q` or the bounds change: `qp::prepare(P, A)` factorises once and the returned `Solver` solves repeatedly. And a warm start and the options (tolerances, the penalty, the iteration cap) are measured, because the benefit of a warm start is what makes this solver useful in model predictive control.

## Program 1: A first QP

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
  const auto q = vec({1.0, 1.0});
  const auto a = mat({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
  const auto lower = vec({1.0, 0.0, 0.0});
  const auto upper = vec({1.0, 0.7, 0.7});
  const auto r = qp::solve(p, q, a, lower, upper);
  const auto x = r.x.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "x          : " << x[0] << ", " << x[1] << "\n";
  std::cout << "objective  : " << r.objective.item<double>() << "\n";
  std::cout << "converged  : " << (r.converged ? "yes" : "no") << " after " << r.iterations << " iterations\n";
  std::cout << std::scientific << std::setprecision(1) << "residuals  : primal " << r.primal_residual << ", dual " << r.dual_residual << "\n";
  const auto y = r.y.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "multipliers: " << y[0] << ", " << y[1] << ", " << y[2] << "  (one per constraint row)\n";
}
```

Output

```text
x          : 0.3000, 0.7000
objective  : 1.8799
converged  : yes after 80 iterations
residuals  : primal 1.7e-05, dual 6.6e-06
multipliers: -2.8999, -0.0000, 0.1999  (one per constraint row)
```
The sum constraint is active (its multiplier is large), `x1` sits at its upper bound (positive multiplier) and `x0`'s bound is slack (multiplier 0). The accuracy is about 1e-4, which is what the default tolerance of 1e-5 on the residuals buys; tighten it for more (Program 4).

## Program 2: The constraint forms

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
  std::cout << std::fixed << std::setprecision(4);
  {
    const auto r = qp::solve(mat({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}), vec({0, 0, 0}), mat({{1, 1, 1}}), vec({1.0}), vec({1.0}));
    const auto x = r.x.host<double>();
    std::cout << "equality    : " << x[0] << ", " << x[1] << ", " << x[2] << "   converged " << r.converged << "\n";
  }
  {
    // 0.5|x - c|^2 = 0.5 x'x - c'x + const, so q = -c
    const auto r = qp::solve(mat({{1, 0}, {0, 1}}), vec({-1.0, -1.0}), mat({{1, 1}}), vec({4.0}), vec({inf}));
    const auto x = r.x.host<double>();
    std::cout << "one-sided   : " << x[0] << ", " << x[1] << "   converged " << r.converged << "\n";
  }
  {
    const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
    const auto r = qp::solve(p, vec({1.0, 1.0}), mat({{1, 0}, {0, 1}}), vec({-inf, -inf}), vec({inf, inf}));
    const auto x = r.x.host<double>();
    std::cout << "unconstrained: " << x[0] << ", " << x[1] << "   (exact " << -1.0 / 7.0 << ", " << -3.0 / 7.0 << ")\n";
  }
}
```

Output

```text
equality    : 0.3333, 0.3333, 0.3333   converged 1
one-sided   : 2.0000, 2.0000   converged 1
unconstrained: -0.1429, -0.4286   (exact -0.1429, -0.4286)
```
An equality, a one-sided bound and an unconstrained problem all use the same call; only the rows and the bounds differ.

## Program 3: Batches and reuse

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

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
  const auto p = mat({{1, 0}, {0, 1}});
  const auto a = mat({{1, 0}, {0, 1}});
  const auto lower = vec({-1.0, -1.0}), upper = vec({1.0, 1.0});
  const auto q = mat({{0.2, -0.5}, {2.0, 0.0}, {-3.0, 3.0}, {-0.9, 0.9}});

  const auto batched = qp::solve(p, q, a, lower, upper);
  std::cout << "batched solve: shape [" << batched.x.size(0) << ", " << batched.x.size(1) << "], converged " << batched.converged << "\n";

  const auto solver = qp::prepare(p, a);
  std::cout << "prepared solver: " << solver.variables() << " variables, " << solver.constraints() << " constraints\n";
  const auto reused = solver.solve(q, lower, upper);

  const auto xb = batched.x.host<double>(), xr = reused.x.host<double>(), qv = q.host<double>();
  std::cout << std::left << std::setw(16) << "q" << std::setw(24) << "x (batched)" << std::setw(24) << "x (prepared)" << "exact\n";
  std::cout << std::fixed << std::setprecision(4);
  for (int i = 0; i < 4; ++i) {
    auto c = [](double v) { return std::max(-1.0, std::min(1.0, v)); };
    std::ostringstream a1, a2, a3, a4;
    a1 << std::fixed << std::setprecision(1) << qv[2 * i] << ", " << qv[2 * i + 1];
    a2 << std::fixed << std::setprecision(4) << xb[2 * i] << ", " << xb[2 * i + 1];
    a3 << std::fixed << std::setprecision(4) << xr[2 * i] << ", " << xr[2 * i + 1];
    a4 << std::fixed << std::setprecision(4) << c(-qv[2 * i]) + 0.0 << ", " << c(-qv[2 * i + 1]) + 0.0;
    std::cout << std::left << std::setw(16) << a1.str() << std::setw(24) << a2.str() << std::setw(24) << a3.str() << a4.str() << "\n";
  }
}
```

Output

```text
batched solve: shape [4, 2], converged 1
prepared solver: 2 variables, 2 constraints
q               x (batched)             x (prepared)            exact
0.2, -0.5       -0.2000, 0.5000         -0.2000, 0.5000         -0.2000, 0.5000
2.0, 0.0        -1.0000, 0.0000         -1.0000, 0.0000         -1.0000, 0.0000
-3.0, 3.0       1.0000, -1.0000         1.0000, -1.0000         1.0000, -1.0000
-0.9, 0.9       0.9000, -0.9000         0.9000, -0.9000         0.9000, -0.9000
```
A `q` of shape `[B, n]` solves `B` problems that share `P`, `A` and the bounds. When only `q` or the bounds change between solves, `qp::prepare(P, A)` checks the problem and factorises it once; the `Solver` it returns solves again and again without that setup, and `variables()` and `constraints()` report its size. Results agree with the one-shot call.

## Program 4: Warm starts and options

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
              << std::scientific << std::setprecision(1) << std::abs(r.x.host<double>()[0] - 0.3) << "\n";
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

## Decisions and pitfalls

- **The default accuracy is about 1e-4.** The default tolerance of 1e-5 on the residuals buys that; tighten it for more (1e-9 doubles the work and gains four digits).
- **Check `converged`.** Stopping at the iteration cap returns the current iterate with `converged` false.
- **A warm start helps when the new problem is close to the old.** A slightly changed problem restarted from the previous `warm()` took 50 iterations against 80 cold.
- **The starting penalty hardly matters with `adaptive_rho` on.** Starts spanning three orders of magnitude all converge in about a hundred iterations; switch the adaptation off and the starting value matters a great deal.
- **Reuse the factorisation.** When only `q` or the bounds change between solves, `qp::prepare` does the setup once.

Where next: That completes the optimization guide. The API reference documents the solvers' types and options.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
