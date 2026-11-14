<!-- generated from bounds_and_constraint_types.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds, inequalities and equalities

[Back to Derivative-Free Optimization 3: Bounds and Constraints](../dfo_03_bounds_and_constraints.md)

Real problems have limits: a dose cannot be negative, a design must carry a load, a budget must balance. Three kinds of limit are available, and which methods accept which is the first thing to know: **bounds** (`lower <= x <= upper`) are accepted by every method; **inequalities** (`g(x) <= 0`, every entry) by pattern search and the mesh methods but not by Nelder–Mead, differential evolution or particle swarm; **equalities** (`h(x) = 0`) only by pattern search. `Auto` routes any problem with constraints to pattern search, the one method that handles all three.

The sign convention matters: a constraint is satisfied when its value is zero or negative, so to require `x0 + x1 >= 1` return `1 - x0 - x1`. The three programs state each kind in turn on small problems whose answers are visible: a box whose optimum is on a corner, a disc, and a curve. Each reports the violation of the returned point, where 0 means feasible.

## Program 1: Bounds only

Every method keeps its trial points inside the box; the objective in this program counts the calls made outside it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  double lowest = 1e300, highest = -1e300;
  std::int64_t outside = 0;
  auto f = [&](const pluss::Series& x) {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a < 0 || a > 2 || b < 0 || b > 2) ++outside;
    lowest = std::min({lowest, a, b});
    highest = std::max({highest, a, b});
    return (x - vec({3.0, -1.0})).square().sum();
  };
  for (auto k : {dfo::DfoSolverKind::NelderMead, dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::DifferentialEvolution,
                 dfo::DfoSolverKind::ParticleSwarm, dfo::DfoSolverKind::OrthoMads}) {
    lowest = 1e300; highest = -1e300; outside = 0;
    dfo::DfoProblem p;
    p.initial = vec({1.0, 1.0});
    p.objective = f;
    p.lower_bounds = vec({0.0, 0.0});
    p.upper_bounds = vec({2.0, 2.0});
    dfo::DfoOptions o;
    o.solver = k;
    o.max_evaluations = 5000;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(24) << dfo::to_string(k) << std::fixed << std::setprecision(4) << "(" << r.minimizer.select(0, 0).item<double>()
              << ", " << r.minimizer.select(0, 1).item<double>() << ")  f " << r.minimum << "  evaluated outside box: " << outside << "\n";
  }
}
```

Output

```text
NelderMead              (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
PatternSearch           (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
DifferentialEvolution   (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
ParticleSwarm           (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
OrthoMads               (2.0000, 0.0000)  f 2.0000  evaluated outside box: 0
```
The optimum sits on a corner, where two bounds are active. All five methods find it. `NelderMead` does it by projecting: the simplex is free to move, but each point is clamped into the box before the objective sees it.

## Program 2: Inequality constraints

`constraints` returns a vector with one entry per constraint.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return (x - vec({2.0, 2.0})).square().sum(); };
  p.constraints = [](const pluss::Series& x) {
    const auto disc = (x.square().sum() - 1.0).reshape({1});
    const auto half = (x.sum() - 3.0).reshape({1});
    return disc.unite({half}, 0);
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << " (" << r.selection_reason << ")\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3.3431)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
```

Output

```text
solver    : PatternSearch (explicit nonlinear constraints detected)
status    : ReachedIterationBudget
minimizer : 0.7059, 0.7083
objective : 3.3432  (exact 3.3431)
violation : 0.0e+00
evals     : 814
```
The answer is on the boundary of the disc, as it must be, since the objective wants to go further out. `violation` is the largest positive constraint value of the returned point; 0 means feasible. The run stopped on its iteration budget while still creeping along the boundary, which is typical of constrained direct search: the last digits are slow, and the objective agrees with the exact value to 1e-4.

## Program 3: Equality constraints

Put them in `constraints_values`, whose result has an `inequality` and an `equality` part; fill either or both.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({3.0, 0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return x.square().sum(); };
  p.constraints_values = [](const pluss::Series& x) {
    dfo::DfoConstraintValues c;
    c.equality = (x.sum() - 3.0).reshape({1});
    return c;
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << "\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << ", " << r.minimizer.select(0, 2).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
```

Output

```text
solver    : PatternSearch
status    : Converged
minimizer : 1.0000, 1.0000, 0.9999
objective : 3.0000  (exact 3)
violation : 1.3e-07
evals     : 1544
```
An equality is harder than an inequality for a method that only compares values: a feasible point must lie on a surface of lower dimension, and a random poll almost never does. Pattern search handles it with an augmented Lagrangian, a penalty that grows with the violation. The violation therefore settles near 1e-7, not at zero; the constraint is met to a tolerance.

## Decisions and pitfalls

- **Every method keeps its trial points in the box.** Even Nelder–Mead, whose simplex is free to move, projects each point into the box before the objective sees it; you will never be called outside the bounds.
- **Constraints are `<= 0`.** A requirement written the other way round is a constraint on the wrong side; the solver will obey it faithfully.
- **An equality is harder than an inequality.** A feasible point lies on a surface of lower dimension, which a random poll almost never hits; pattern search uses an augmented Lagrangian, so the violation shrinks as the run proceeds rather than being zero at once.
- **Stopping on the budget while creeping along a boundary is typical.** Check the violation and raise the budget if you need more digits.
- **Check feasibility yourself.** Recompute the constraints at the returned point.

Where next: [Mesh methods and infeasible problems](mesh_methods_and_infeasible_problems.md) covers the other constrained solvers and what happens when no point is feasible.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="mesh_methods_and_infeasible_problems.html">
    <span class="label">Next</span>
    <span class="title">Mesh methods with constraints, and infeasible problems</span>
  </a>
</div>
<!-- apiref:nav:end -->
