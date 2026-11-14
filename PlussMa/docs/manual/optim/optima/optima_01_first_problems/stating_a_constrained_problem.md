<!-- generated from stating_a_constrained_problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stating a constrained problem

[Back to Optima 1: Your First Constrained Problems](../optima_01_first_problems.md)

A constrained problem has two parts: a quantity to make small, and rules the answer must obey. `optima` takes both in one call, and the whole skill of using it is learning the vocabulary for the rules. This page builds that vocabulary one constraint family at a time, on problems small enough that the right answer is visible: with no constraints at all, then with bounds, a nonlinear inequality, a nonlinear equality and linear constraints, and finally with all of them together.

Read each program for two things. First, the *shape of the call*: the objective is a function written with ordinary `Series` operations, which record how a number was computed so that the solver gets exact gradients without you deriving them; the start is a `Series`; the constraints are bundled with `constraints()` and passed as the fourth argument. Second, the *shape of the answer*: where the solver ends up relative to the unconstrained minimum tells you which constraints are active, and an active constraint is the one that is shaping the answer.

## Program 1: The smallest call

Start with no constraints at all. The objective is the squared distance to a target point, so the answer is the target. This shows the whole shape of a call: an objective written with `Series` operations, a starting `Series`, and a result you read by block name.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;

  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}).to(f64);
  const auto start = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64);

  // f(x) = |x - target|^2, minimized where x == target.
  const auto result = optim::optima(
      [&](const pluss::Series& x) { return (x - target).square().sum(); },
      start);

  const auto x = result.params.at("x").host<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "iterations: " << result.iteration << "\n";
  return 0;
}
```

Output

```text
x = [1, 2]
converged: true
exit flag: 1
iterations: 1
```
The solver reaches the target `(1, 2)` to within its tolerance, reports `converged: true`, and exits with flag `1`, which is the code for `Converged`. The answer is accurate to the stopping tolerance, not guaranteed to the last digit, so always judge a result against the tolerance rather than against an exact printed value.

## Program 2: Bounds

Bounds restrict each coordinate to an interval. This program puts the unconstrained minimum at `(3, -2)` and the box at `[0, 2] x [0, 2]`, so the solver has to settle on the corner of the box nearest the target. Bounds go into a `constraints()` value, passed as the fourth argument.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, -2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };

  const auto box = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({2.0, 2.0}));
  const auto result = optim::optima(objective, vec({1.0, 1.0}), {}, box);

  const auto x = result.params.at("x").host<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective = " << result.evaluation.value.item<double>() << "\n";
  std::cout << "max violation = " << result.evaluation.max_constraint_violation << "\n";
  return 0;
}
```

Output

```text
x = [2, 7.18148e-12]
objective = 5
max violation = 0
```
The answer is the corner `(2, 0)`. The second coordinate prints as `7.18148e-12` instead of exactly `0`: a coordinate that should sit on a bound can come back a hair away from it, within the stopping tolerance. The reported violation of `0` confirms the point is feasible.

## Program 3: A nonlinear inequality

A nonlinear inequality returns `g(x)`, required to be at most zero. Here `g(x) = x0^2 + x1^2 - 1`, so the answer must lie inside the unit disc. The target `(1, 2)` is outside the disc, so the best feasible point is the point of the disc closest to the target, on the circle.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };

  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<pluss::Series>& p) { return p.at("x").square().sum() - 1.0; });
  const auto result = optim::optima(objective, vec({0.0, 0.0}), {}, disc);

  const auto x = result.params.at("x").host<double>();
  const double norm = std::sqrt(x[0] * x[0] + x[1] * x[1]);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "distance from origin = " << norm << "\n";
  std::cout << "closest disc point to (1, 2) is (1, 2)/sqrt(5) = [" << 1.0 / std::sqrt(5.0) << ", "
            << 2.0 / std::sqrt(5.0) << "]\n";
  return 0;
}
```

Output

```text
x = [0.447214, 0.894427]
distance from origin = 1
closest disc point to (1, 2) is (1, 2)/sqrt(5) = [0.447214, 0.894427]
```
The solver lands on the circle at the direction of the target, matching `(1, 2) / sqrt(5)` to five digits. When the unconstrained minimum is outside the feasible region, the answer sits on the boundary, and the constraint is said to be active there.

### Watching the iterations

Every option struct has a `progress_hook`, a function the solver calls after each iteration. `reports::iter(std::cout, 4)` is a ready-made hook that prints one line per iteration, with four decimals. The same disc problem is solved again with it switched on.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<pluss::Series>& p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.progress_hook = optim::reports::iter(std::cout, 4);  // four decimals per number
  const auto result = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << "converged: " << std::boolalpha << result.converged << " after " << result.iteration << " iterations\n";
  return 0;
}
```

Output

```text
iter=1 evals=5 objective=1.2500 viol=0.2500 stationarity=1.0000 backend=5.0000 step=0.2500 stall=none
iter=2 evals=6 objective=1.5125 viol=0.0125 stationarity=0.0500 backend=2.5000 step=1.0000 stall=none
iter=3 evals=7 objective=1.5278 viol=0.0000 stationarity=0.0058 backend=1.3687 step=1.0000 stall=none
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 stall=none
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 stall=none
converged: true after 4 iterations
```
Read the lines from the top. After the first step (iteration 1) the point is slightly outside the disc, with a violation of 0.25; the violation falls to 0.0125 at iteration 2 and to zero at iteration 3, while the objective rises from 1.25 to 1.5279 because the solver is being pulled onto the circle that bounds the disc. The stationarity measure (how far the point is from satisfying the optimality conditions) falls from 1 to 0.05, 0.0058 and then to zero, at which point the run stops. The last line repeats iteration 4: the solver reports the converged point once more after its convergence test, and the message that explains why it stopped is the part this short form leaves out (`reports::trace` prints it; see "Watching a run").

## Program 4: A nonlinear equality

An equality `h(x) = 0` forces the answer onto a curve. This program asks for the point on the unit circle nearest `(3, 3)`, which is `(1/sqrt 2, 1/sqrt 2)`.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <iomanip>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, 3.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };

  const auto circle = optim::constraints().nl_eq(
      [](const optim::ParameterPack<pluss::Series>& p) { return p.at("x").square().sum() - 1.0; });
  const auto result = optim::optima(objective, vec({1.0, 0.0}), {}, circle);

  const auto x = result.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "x^2 + y^2 = " << x[0] * x[0] + x[1] * x[1] << "\n";
  std::cout << "expected 1/sqrt(2) = " << 0.70710678 << "\n";
  return 0;
}
```

Output

```text
x = [0.7071, 0.7071]
x^2 + y^2 = 1.0000
expected 1/sqrt(2) = 0.7071
```
The answer is `(0.7071, 0.7071)`, matching the exact `(0.70711, 0.70711)` to four decimals, and `x^2 + y^2` is `1` to four decimals. Unlike an inequality, an equality is always active.

## Program 5: Linear constraints

Linear constraints are matrices. `ln_eq(Aeq, beq)` means `Aeq x = beq` and `ln_ineq(Aineq, bineq)` means `Aineq x <= bineq`, row by row. Each matrix has one column per unknown. This program keeps three numbers summing to 3 while preferring them near 2, and also requires `x1 - x2 <= -0.5`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = vec({2.0, 2.0, 2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };

  const auto constraints = optim::constraints()
                               .ln_eq(mat({1, 1, 1}, 1, 3), vec({3.0}))
                               .ln_ineq(mat({1, -1, 0}, 1, 3), vec({-0.5}));
  const auto result = optim::optima(objective, vec({0.0, 0.0, 0.0}), {}, constraints);

  const auto x = result.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << ", " << x[2] << "]\n";
  std::cout << "sum = " << x[0] + x[1] + x[2] << " (equality target 3)\n";
  std::cout << "x1 - x2 = " << x[0] - x[1] << " (inequality limit -0.5)\n";
  return 0;
}
```

Output

```text
x = [0.7500, 1.2500, 1.0000]
sum = 3.0000 (equality target 3)
x1 - x2 = -0.5000 (inequality limit -0.5)
```
The sum is exactly the target, and `x1 - x2` sits at its limit of `-0.5`: the inequality is active, which is why the answer is not the symmetric point `(1, 1, 1)`.

## Program 6: Every constraint family at once

Real problems mix constraint types. This program has three scalar unknowns named `a`, `b` and `c` and uses all five families in one chain. Because the unknowns are named blocks, bounds are given as packs with the same names. Linear rows address the blocks in order, one column per scalar.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };
  using Pack = optim::ParameterPack<pluss::Series>;

  // Minimize a^2 + 2 b^2 + 3 c^2 - 4 a.
  auto objective = [](const Pack& p) {
    return p.at("a").square() + 2.0 * p.at("b").square() + 3.0 * p.at("c").square() -
           4.0 * p.at("a");
  };

  const auto constraints =
      optim::constraints()
          .lbs(optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}, {"c", scalar(0.0)}}))
          .ubs(optim::pack_scalars({{"a", scalar(1.5)}, {"b", scalar(2.0)}, {"c", scalar(2.0)}}))
          .ln_eq(mat({0, 1, 1}, 1, 3), vec({1.0}))                         // b + c = 1
          .ln_ineq(mat({1, 1, 1}, 1, 3), vec({2.5}))                       // a + b + c <= 2.5
          .nl_eq([](const Pack& p) { return p.at("a") * p.at("b") - 0.3; })  // a * b = 0.3
          .nl_ineq([](const Pack& p) { return p.at("c").square() - 0.6; });  // c^2 <= 0.6

  const auto start = optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.5)}, {"c", scalar(0.5)}});
  const auto result = optim::optima(objective, start, {}, constraints);

  const double a = as_f64(result.params.at("a"));
  const double b = as_f64(result.params.at("b"));
  const double c = as_f64(result.params.at("c"));
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "a = " << a << ", b = " << b << ", c = " << c << "\n";
  std::cout << "b + c = " << b + c << " (target 1)\n";
  std::cout << "a * b = " << a * b << " (target 0.3)\n";
  std::cout << "c^2 = " << c * c << " (limit 0.6)\n";
  std::cout << "max violation below 1e-5: " << std::boolalpha << (result.evaluation.max_constraint_violation <= 1e-5)
            << "\n";
  return 0;
}
```

Output

```text
a = 1.3309, b = 0.2254, c = 0.7746
b + c = 1.0000 (target 1)
a * b = 0.3000 (target 0.3)
c^2 = 0.6000 (limit 0.6)
max violation below 1e-5: true
```
All the requirements hold at the answer. The constraint `c^2 <= 0.6` is active, since `c^2` prints as exactly its limit. The solver chose the largest allowed `c` because the objective rewards it.

## Decisions and pitfalls

- **Inequalities are written "at most zero".** A nonlinear inequality returns `g(x)` and holds when `g(x) <= 0`; to require `x0 + x1 <= 1.5`, return `x0 + x1 - 1.5`. Writing it the other way round constrains the wrong side and the solver will happily obey.
- **An answer sits on a boundary only when the constraint is active.** If the unconstrained minimum is already feasible, the constraints are inactive and change nothing; check which ones are active before reading the result as "limited by" them.
- **Tolerances, not equalities.** A coordinate that should be `0` may come back as `1e-12`, and an equality holds to about `1e-6`. Judge feasibility with the reported violation, not by comparing doubles with `==`.
- **Linear constraints are matrices with one column per unknown.** `ln_eq(Aeq, beq)` and `ln_ineq(Aineq, bineq)` address the unknowns in order; a row with the wrong number of columns is a programming error, not a modelling one.
- **Several constraint types at once is normal.** Put them all in one `constraints()` value; the solver treats the families together.

Where next: [Blocks, problems as values, and reading the result](blocks_values_and_reading_the_result.md) moves from vectors of anonymous unknowns to named quantities.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="blocks_values_and_reading_the_result.html">
    <span class="label">Next</span>
    <span class="title">Blocks, problems as values, gradients and reading the result</span>
  </a>
</div>
<!-- apiref:nav:end -->
