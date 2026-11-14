<!-- generated from four_algorithms_on_one_problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Four algorithms on one problem

[Back to Optima 3: Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)

`optima` offers four algorithms for the same problem class: a merit method, sequential quadratic programming (SQP, the default), an active-set (working-set) method and an interior-point method. They share the problem description and the result type, so switching is a one-word change; what differs is the *path* each takes and what each is good at. The honest way to learn the differences is to run all four on the same problems and compare, which is what this page does.

The four programs ask four different questions of that comparison. How do the methods compare on effort and on the answer they reach? Does an algorithm keep its iterates feasible on the way (which matters if your objective is undefined outside the feasible region)? On a problem with several local minima, which minimum does each reach from each start? And what happens when the objective returns `NaN` or infinity, as real models do outside their domain?

## Program 1: The same problem, four algorithms

The first program solves one problem with all four algorithms. It has a nonlinear equality, a nonlinear inequality and bounds, and it is started on the boundary of the inequality and on two bounds, which is a deliberately awkward start. The table has the same columns as the report from chapter 2, so every number can be compared across rows.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };

  // minimize a*d*(a + b + c) + c
  //   subject to a*b*c*d >= 25, a^2 + b^2 + c^2 + d^2 = 40, 1 <= each <= 5
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return p.at("a").square() + p.at("b").square() + p.at("c").square() +
                   p.at("d").square() - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  optim::OptimaState merit_result;
  std::cout << std::left << std::setw(17) << "algorithm" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(14) << "objective" << "violation\n";
  for (const auto& row : rows) {
    optim::OptimaOptions options;
    options.algorithm = row.algorithm;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints);
    if (row.algorithm == optim::OptimaAlgorithm::MeritMethod) merit_result = r;
    std::cout << std::left << std::setw(17) << row.label << std::setw(6) << static_cast<int>(r.exit_flag)
              << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed
              << std::setprecision(6) << std::setw(14) << r.evaluation.value.item<double>() << std::scientific
              << std::setprecision(2) << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  }
  std::cout << std::fixed << std::setprecision(4) << "\nMeritMethod ended at a = "
            << as_f64(merit_result.params.at("a")) << ", b = " << as_f64(merit_result.params.at("b"))
            << ", c = " << as_f64(merit_result.params.at("c")) << ", d = "
            << as_f64(merit_result.params.at("d")) << " (a is at its lower bound 1, b at its upper bound 5)\n";
  return 0;
}
```

Output

```text
algorithm        flag  iters  evals  objective     violation
MeritMethod      1     68     180    27.146428     1.32e-09
SequentialQP     1     4      5      17.014017     8.24e-08
WorkingSet       1     4      6      17.014017     8.24e-08
InternalProtocol 1     8      12     17.014018     8.84e-08

MeritMethod ended at a = 1.0000, b = 5.0000, c = 1.4495, d = 3.4495 (a is at its lower bound 1, b at its upper bound 5)
```
All four exit with flag `1`, but they do not agree. Sequential QP, the working-set method and the interior method reach `17.0140`, the best known value for this problem, in 4, 4 and 8 iterations. The merit method needs 68 iterations and ends at `27.1464`. That second value is not a failure: the last line shows it is a different point, with `a` on its lower bound and `b` on its upper bound, and the solver certifies it as a local solution. A flag of `1` means "a local solution", and this table is the reason to remember it. The same row-by-row difference, fewer iterations for the quadratic-model methods, holds across the rest of the chapter.

## Program 2: What happens to the constraints on the way

An algorithm that is cheap overall can still be unusable if your objective is undefined where the constraints are violated. This program records the constraint violation at every iteration and counts how many iterates were infeasible, from a start inside the feasible region and from a start outside it.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize (a - 3)^2 + (b - 3)^2 subject to a^2 + b^2 <= 2 (a disc around the origin)
  auto objective = [](const Pack& p) {
    return (p.at("a") - 3.0).square() + (p.at("b") - 3.0).square();
  };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("a").square() + p.at("b").square() - 2.0; });

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  for (const auto& start : {std::pair{"inside the disc at (0, 0)", two(0, 0)},
                            std::pair{"outside the disc at (3, 3)", two(3, 3)}}) {
    std::cout << "start " << start.first << "\n";
    for (const auto& row : rows) {
      std::vector<double> violation;
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
        if (s.stage == optim::OptimaProgressStage::Iteration)
          violation.push_back(s.evaluation.max_constraint_violation);
        return false;
      };
      const auto r = optim::optima(objective, start.second, options, disc);
      const auto infeasible = std::count_if(violation.begin(), violation.end(), [](double v) { return v > 1e-6; });
      std::cout << "  " << std::left << std::setw(17) << row.label << "iterations " << std::setw(3)
                << violation.size() << " infeasible iterates " << std::setw(3) << infeasible << " flag "
                << static_cast<int>(r.exit_flag) << "\n";
    }
  }
  return 0;
}
```

Output

```text
start inside the disc at (0, 0)
  MeritMethod      iterations 18  infeasible iterates 13  flag 1
  SequentialQP     iterations 6   infeasible iterates 4   flag 1
  WorkingSet       iterations 9   infeasible iterates 0   flag 1
  InternalProtocol iterations 6   infeasible iterates 2   flag 1
start outside the disc at (3, 3)
  MeritMethod      iterations 21  infeasible iterates 18  flag 1
  SequentialQP     iterations 6   infeasible iterates 4   flag 1
  WorkingSet       iterations 12  infeasible iterates 2   flag 1
  InternalProtocol iterations 7   infeasible iterates 6   flag 1
```
No algorithm here promises feasible iterates. The merit method is infeasible at most iterations because it trades violation against the objective until the end. Sequential QP crosses the boundary on four iterations, and the interior method on two or six, because by default each nonlinear inequality has its own slack variable (Program 7 shows what that buys and how to switch it off). The working-set method is the most conservative here: no infeasible iterate from the feasible start, two from the infeasible one. If your objective cannot be evaluated outside the feasible set, use `InternalProtocol` with `independent_nonlinear_slacks = false`, which keeps every inequality strictly satisfied, and test it on your problem rather than taking it on trust.

## Program 3: Where you start decides where you end

A nonconvex objective has several local minima, and each algorithm falls into one of them. This objective, `(x^2 - 1)^2 + 0.3 x`, has a deep minimum near `-1.04` and a shallow one near `0.96`. The table shows the point each algorithm reports from four starts.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto one = [&](double x) { return optim::pack_scalars({{"x", scalar(x)}}); };

  auto objective = [](const Pack& p) {
    return (p.at("x").square() - 1.0).square() + 0.3 * p.at("x");
  };
  const auto box = optim::constraints().lbs(one(-2.0)).ubs(one(2.0));

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  const std::vector<double> starts = {-1.5, -0.3, 0.3, 1.5};

  int converged = 0;
  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::left << std::setw(18) << "start";
  for (double s : starts) std::cout << std::right << std::setw(9) << s;
  std::cout << "\n";
  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (double s : starts) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      const auto r = optim::optima(objective, one(s), options, box);
      std::cout << std::right << std::setw(9) << as_f64(r.params.at("x"));
      if (r.converged) ++converged;
    }
    std::cout << "\n";
  }
  std::cout << "runs that exited with flag Converged: " << converged << " of 16\n";
  return 0;
}
```

Output

```text
start                -1.500   -0.300    0.300    1.500
MeritMethod          -1.036   -1.036    0.960    0.960
SequentialQP          0.960   -1.036    0.960   -1.036
WorkingSet            0.960   -1.036    0.960   -1.036
InternalProtocol      0.960   -1.036    0.960   -1.036
runs that exited with flag Converged: 16 of 16
```
The last line confirms that all 16 runs exit with flag `Converged`. The merit method stays where it starts: the two left starts end at `-1.036`, the two right starts at `0.960`. The three quadratic-model methods jump across the barrier between the wells on the far starts, so the two outer starts, `-1.5` and `1.5`, each end up in the other well. The deep minimum at `-1.036` is reached from `-0.3` and from `1.5` by those three methods, and the shallow minimum from `-1.5` and `0.3`. There is no rule that a start close to a minimum reaches it, so a model with several minima needs several starts, and the answer to compare across them is the objective value, never the exit flag.

## Program 4: When the objective is not a number

Real models return `NaN` or infinity in regions the optimizer should never have visited: a log of a negative number, a variance that has gone negative. The three cases below are the ones that matter: a start with no value, a region of `NaN`, and a wall of `+infinity`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Series = pluss::Series;
const auto f64 = pluss::ScalarType::Float64;

// f(x) = 100 (x - 3)^2 for x > bound, otherwise `outside` (NaN or +infinity), with the exact gradient.
optim::ConstrainedProblem wall(double bound, double outside) {
  optim::ConstrainedProblem problem;
  problem.objective.analytic = [bound, outside](const optim::ParameterPack<Series>& p)
      -> std::pair<Series, optim::ParameterPack<Series>> {
    const double x = as_f64(p.at("x"));
    if (!(x > bound)) {
      return {pluss::make_series(outside).to(f64),
              optim::ParameterPack<Series>({{"x", pluss::make_series(outside).to(f64).reshape({1})}})};
    }
    return {pluss::make_series(100.0 * (x - 3.0) * (x - 3.0)).to(f64),
            optim::ParameterPack<Series>({{"x", pluss::make_series(200.0 * (x - 3.0)).to(f64).reshape({1})}})};
  };
  return problem;
}

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  auto start = [&](double x) { return optim::pack_scalars({{"x", pluss::make_series(x).to(f64)}}); };

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  struct Case { const char* label; optim::ConstrainedProblem problem; double x0; };
  const std::vector<Case> cases = {{"NaN at the start", wall(100.0, nan), 4.0},
                                   {"NaN region below x = 2.5", wall(2.5, nan), 4.0},
                                   {"+inf wall below x = 2.5", wall(2.5, inf), 4.0}};

  std::cout << std::fixed << std::setprecision(4);
  for (const auto& c : cases) {
    std::cout << c.label << "\n";
    for (const auto& row : rows) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      const auto r = optim::optima(c.problem, start(c.x0), options);
      std::cout << "  " << std::left << std::setw(17) << row.label << "flag " << std::setw(3)
                << static_cast<int>(r.exit_flag) << "x = " << as_f64(r.params.at("x")) << "\n";
    }
  }
  return 0;
}
```

Output

```text
NaN at the start
  MeritMethod      flag -4 x = 4.0000
  SequentialQP     flag -4 x = 4.0000
  WorkingSet       flag -4 x = 4.0000
  InternalProtocol flag -4 x = 4.0000
NaN region below x = 2.5
  MeritMethod      flag 1  x = 3.0000
  SequentialQP     flag 1  x = 3.0000
  WorkingSet       flag 1  x = 3.0000
  InternalProtocol flag 1  x = 3.0000
+inf wall below x = 2.5
  MeritMethod      flag 1  x = 3.0000
  SequentialQP     flag 1  x = 3.0000
  WorkingSet       flag 1  x = 3.0000
  InternalProtocol flag 1  x = 3.0000
```
A start without a value is a failure for all four, flag `-4`, and the start is returned untouched: no algorithm can step from a point it cannot evaluate. In the other two cases every algorithm backs off the bad region and converges to `x = 3`. This is the behaviour to rely on when an optimizer wanders into a bad region. It does not make a start without a value usable.

## Decisions and pitfalls

- **Converged does not mean the same point.** Different methods can all report `Converged` at different local minima; compare objective values, not just flags.
- **No method promises feasible iterates by default.** If your objective is undefined outside the feasible set, use the interior method with `independent_nonlinear_slacks = false`, and test it on your problem rather than taking it on trust.
- **A nonconvex problem needs several starts.** The method decides which basin you fall into as much as the start does; run from a spread of starting points and keep the best.
- **Back-off from a bad region is not the same as a usable start.** Every algorithm recovers from `NaN` or an infinite wall met *during* a run, but none can start from a point whose value does not exist.
- **Pick by the problem, then confirm.** SQP is the default for a reason (few evaluations on smooth problems); the working-set and interior methods earn their place on problems with many active inequalities.

Where next: [Curvature, scaling and size](curvature_scaling_and_size.md) looks at what controls the cost of a step.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="curvature_scaling_and_size.html">
    <span class="label">Next</span>
    <span class="title">Curvature, scaling and size</span>
  </a>
</div>
<!-- apiref:nav:end -->
