<!-- generated from optima_01_first_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 1: Your First Constrained Problems

You have a quantity to make as small as possible, and some rules the answer must obey: it has to stay inside a box, on one side of a curve, or satisfy a few linear equations. This chapter shows how to state such a problem to `optima`, how to read what comes back, and which of the call shapes to choose. By the end you can solve a problem with any mix of bounds, linear constraints and nonlinear constraints, with or without hand-written gradients.

Every program in this chapter is complete. Copy one into a file, add `#include <pluss/ma.h>` if it is not already there, and it builds as shown. The output under each program is the exact output of that program.

## The mental model

`optima` solves

```text
minimize    f(x)
subject to  lower <= x <= upper              (bounds)
            Aeq x  = beq                     (linear equalities)
            Aineq x <= bineq                 (linear inequalities)
            h(x)   = 0                       (nonlinear equalities)
            g(x)  <= 0                       (nonlinear inequalities)
```

Four ideas carry the rest of the chapter.

- **The unknown is a pack of named blocks.** A problem with one vector of unknowns has a single block called `x`. A problem with several quantities of different shapes uses one block per quantity, such as `slope` and `intercept`. Blocks keep their names and shapes in the answer, so you read results back by name.
- **Derivatives are exact and automatic.** You write `f`, `g` and `h` with the `Trace` type, a number that remembers how it was computed. The solver then has exact gradients and Jacobians without you deriving them. If you already have a gradient, you can hand it over instead (program 9).
- **Inequalities are written "at most zero".** A nonlinear inequality returns `g(x)`, and the constraint holds when `g(x) <= 0`. To require `x0 + x1 <= 1.5`, return `x0 + x1 - 1.5`. Linear inequalities use the matrix form `Aineq x <= bineq`.
- **Answers satisfy tolerances, not equalities.** The solver stops when constraints are met to within `1e-6` and the optimality measure is below `1e-6`. A coordinate that should be exactly `0` may come back as `1e-7`. Program 2 shows this.

## The six call shapes

`optima` has six overloads. They differ only in how you hand over the objective and the starting point.

| Objective | Start | Use when |
|---|---|---|
| `Trace` function of one vector `Trace` | one `Series` | one vector of unknowns (programs 1 to 5) |
| `Trace` function of a pack | `ParameterPack<Series>` | several named blocks (programs 6, 7, 11) |
| a `ConstrainedProblem` | one `Series` | the problem is built once and reused (program 8) |
| a `ConstrainedProblem` | `ParameterPack<Series>` | the problem is reused and has named blocks (program 8) |
| value-and-gradient function of one `Series` | one `Series` | you have an analytic gradient (program 9) |
| value-and-gradient function of a pack | `ParameterPack<Series>` | analytic gradient, named blocks (program 9) |

The four overloads that take a function (the first, second, fifth and sixth rows) accept the constraints as an optional fourth argument, after the options: a value built with `constraints()`. A `ConstrainedProblem` already contains its constraints, so the two overloads that take one have no such argument.

## A guided tour

### Program 1: the smallest call

Start with no constraints at all. The objective is the squared distance to a target point, so the answer is the target. This shows the whole shape of a call: an objective written with `Trace`, a starting `Series`, and a result you read by block name.

```cpp
// The smallest useful call: minimize a smooth function of one vector, with no constraints.
// The objective receives the parameters as a Trace, so the solver gets exact derivatives
// without you writing any.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;

  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}).to(f64);
  const auto start = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64);

  // f(x) = |x - target|^2, minimized where x == target.
  const auto result = optim::optima(
      [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); },
      start);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "iterations: " << result.iteration << "\n";
  return 0;
}
```

### Output

```text
x = [1, 2]
converged: true
exit flag: 1
iterations: 1
```
The solver reaches the target `(1, 2)` to within its tolerance, reports `converged: true`, and exits with flag `1`, which is the code for `Converged`. The answer is accurate to the stopping tolerance, not guaranteed to the last digit, so always judge a result against the tolerance rather than against an exact printed value.

### Program 2: bounds

Bounds restrict each coordinate to an interval. This program puts the unconstrained minimum at `(3, -2)` and the box at `[0, 2] x [0, 2]`, so the solver has to settle on the corner of the box nearest the target. Bounds go into a `constraints()` value, passed as the fourth argument.

```cpp
// Bounds restrict each coordinate to an interval. The unconstrained minimum below sits at
// (3, -2); with 0 <= x <= 2 and 0 <= y <= 2 the answer is the nearest corner of the box.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, -2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto box = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({2.0, 2.0}));
  const auto result = optim::optima(objective, vec({1.0, 1.0}), {}, box);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective = " << result.evaluation.value.item<double>() << "\n";
  std::cout << "max violation = " << result.evaluation.max_constraint_violation << "\n";
  return 0;
}
```

### Output

```text
x = [2, 7.18148e-12]
objective = 5
max violation = 0
```
The answer is the corner `(2, 0)`. The second coordinate prints as `7.18148e-12` instead of exactly `0`: a coordinate that should sit on a bound can come back a hair away from it, within the stopping tolerance. The reported violation of `0` confirms the point is feasible.

### Program 3: a nonlinear inequality

A nonlinear inequality returns `g(x)`, required to be at most zero. Here `g(x) = x0^2 + x1^2 - 1`, so the answer must lie inside the unit disc. The target `(1, 2)` is outside the disc, so the best feasible point is the point of the disc closest to the target, on the circle.

```cpp
// A nonlinear inequality constraint g(x) <= 0 keeps the answer on one side of a curve.
// Here the unconstrained minimum (1, 2) lies outside the disc x^2 + y^2 <= 1, so the
// solution lands on the circle, at the point of the disc closest to (1, 2).
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<optim::Trace>& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto result = optim::optima(objective, vec({0.0, 0.0}), {}, disc);

  const auto x = result.params.at("x").to_vec<double>();
  const double norm = std::sqrt(x[0] * x[0] + x[1] * x[1]);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "distance from origin = " << norm << "\n";
  std::cout << "closest disc point to (1, 2) is (1, 2)/sqrt(5) = [" << 1.0 / std::sqrt(5.0) << ", "
            << 2.0 / std::sqrt(5.0) << "]\n";
  return 0;
}
```

### Output

```text
x = [0.447214, 0.894427]
distance from origin = 1
closest disc point to (1, 2) is (1, 2)/sqrt(5) = [0.447214, 0.894427]
```
The solver lands on the circle at the direction of the target, matching `(1, 2) / sqrt(5)` to five digits. When the unconstrained minimum is outside the feasible region, the answer sits on the boundary, and the constraint is said to be active there.

### Program 4: a nonlinear equality

An equality `h(x) = 0` forces the answer onto a curve. This program asks for the point on the unit circle nearest `(3, 3)`, which is `(1/sqrt 2, 1/sqrt 2)`.

```cpp
// A nonlinear equality constraint h(x) = 0 forces the answer onto a curve. Minimizing the
// distance to (3, 3) while staying on the unit circle picks the circle point facing (3, 3).
#include <pluss/ma.h>

#include <iostream>
#include <iomanip>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, 3.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto circle = optim::constraints().nl_eq(
      [](const optim::ParameterPack<optim::Trace>& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto result = optim::optima(objective, vec({1.0, 0.0}), {}, circle);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "x^2 + y^2 = " << x[0] * x[0] + x[1] * x[1] << "\n";
  std::cout << "expected 1/sqrt(2) = " << 0.70710678 << "\n";
  return 0;
}
```

### Output

```text
x = [0.7071, 0.7071]
x^2 + y^2 = 1.0000
expected 1/sqrt(2) = 0.7071
```
The answer is `(0.7071, 0.7071)`, matching the exact `(0.70711, 0.70711)` to four decimals, and `x^2 + y^2` is `1` to four decimals. Unlike an inequality, an equality is always active.

### Program 5: linear constraints

Linear constraints are matrices. `ln_eq(Aeq, beq)` means `Aeq x = beq` and `ln_ineq(Aineq, bineq)` means `Aineq x <= bineq`, row by row. Each matrix has one column per unknown. This program keeps three numbers summing to 3 while preferring them near 2, and also requires `x1 - x2 <= -0.5`.

```cpp
// Linear constraints are given as matrices. ln_ineq(A, b) means A x <= b row by row, and
// ln_eq(A, b) means A x = b. Minimize |x - (2, 2, 2)|^2 subject to x1 + x2 + x3 = 3 (equality)
// and x1 - x2 <= -0.5 (inequality).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = vec({2.0, 2.0, 2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto constraints = optim::constraints()
                               .ln_eq(mat({1, 1, 1}, 1, 3), vec({3.0}))
                               .ln_ineq(mat({1, -1, 0}, 1, 3), vec({-0.5}));
  const auto result = optim::optima(objective, vec({0.0, 0.0, 0.0}), {}, constraints);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << ", " << x[2] << "]\n";
  std::cout << "sum = " << x[0] + x[1] + x[2] << " (equality target 3)\n";
  std::cout << "x1 - x2 = " << x[0] - x[1] << " (inequality limit -0.5)\n";
  return 0;
}
```

### Output

```text
x = [0.7500, 1.2500, 1.0000]
sum = 3.0000 (equality target 3)
x1 - x2 = -0.5000 (inequality limit -0.5)
```
The sum is exactly the target, and `x1 - x2` sits at its limit of `-0.5`: the inequality is active, which is why the answer is not the symmetric point `(1, 1, 1)`.

### Program 6: every constraint family at once

Real problems mix constraint types. This program has three scalar unknowns named `a`, `b` and `c` and uses all five families in one chain. Because the unknowns are named blocks, bounds are given as packs with the same names. Linear rows address the blocks in order, one column per scalar.

```cpp
// Every constraint family can be combined in one builder chain: bounds, linear equality and
// inequality, and nonlinear equality and inequality. The unknowns are three named scalar
// blocks a, b, c. Bounds are given as packs with the same block names; linear rows address
// the blocks in order, one column per scalar.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };
  using Pack = optim::ParameterPack<optim::Trace>;

  // Minimize a^2 + 2 b^2 + 3 c^2 - 4 a.
  auto objective = [](const Pack& p) {
    return optim::square(p.at("a")) + 2.0 * optim::square(p.at("b")) + 3.0 * optim::square(p.at("c")) -
           4.0 * p.at("a");
  };

  const auto constraints =
      optim::constraints()
          .lbs(optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}, {"c", scalar(0.0)}}))
          .ubs(optim::pack_scalars({{"a", scalar(1.5)}, {"b", scalar(2.0)}, {"c", scalar(2.0)}}))
          .ln_eq(mat({0, 1, 1}, 1, 3), vec({1.0}))                         // b + c = 1
          .ln_ineq(mat({1, 1, 1}, 1, 3), vec({2.5}))                       // a + b + c <= 2.5
          .nl_eq([](const Pack& p) { return p.at("a") * p.at("b") - 0.3; })  // a * b = 0.3
          .nl_ineq([](const Pack& p) { return optim::square(p.at("c")) - 0.6; });  // c^2 <= 0.6

  const auto start = optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.5)}, {"c", scalar(0.5)}});
  const auto result = optim::optima(objective, start, {}, constraints);

  const double a = result.params.at("a").item<double>();
  const double b = result.params.at("b").item<double>();
  const double c = result.params.at("c").item<double>();
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

### Output

```text
a = 1.3309, b = 0.2254, c = 0.7746
b + c = 1.0000 (target 1)
a * b = 0.3000 (target 0.3)
c^2 = 0.6000 (limit 0.6)
max violation below 1e-5: true
```
All the requirements hold at the answer. The constraint `c^2 <= 0.6` is active, since `c^2` prints as exactly its limit. The solver chose the largest allowed `c` because the objective rewards it.

### Program 7: named blocks and a model

Named blocks shine when the unknowns mean something. This program fits a straight line to five points. The objective reads like the model, with `slope` and `intercept` as separate blocks, and the answer is read back by those names.

```cpp
// Parameters can be split into named blocks of any shape, so an objective reads like the
// model it describes. Here a line y = slope * t + intercept is fitted to five points: the
// unknowns are two blocks, "slope" and "intercept", each a single number.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  using Pack = optim::ParameterPack<optim::Trace>;

  const auto t = optim::trace(vec({0.0, 1.0, 2.0, 3.0, 4.0}), false);
  const auto y = optim::trace(vec({1.1, 2.9, 5.2, 7.1, 8.8}), false);

  auto sum_squared_error = [&](const Pack& p) {
    const auto prediction = p.at("slope") * t + p.at("intercept");
    return optim::sum(optim::square(prediction - y));
  };

  const auto start = optim::pack_scalars({{"slope", pluss::make_series(0.0).to(f64)},
                                          {"intercept", pluss::make_series(0.0).to(f64)}});
  const auto result = optim::optima(sum_squared_error, start);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "slope     = " << result.params.at("slope").item<double>() << "\n";
  std::cout << "intercept = " << result.params.at("intercept").item<double>() << "\n";
  std::cout << "blocks returned: " << result.params.size() << "\n";
  return 0;
}
```

### Output

```text
slope     = 1.9600
intercept = 1.1000
blocks returned: 2
```
The slope `1.9601` and intercept `1.0999` match the closed-form least-squares line for these data, `1.96` and `1.10`, to the solver tolerance. The result keeps both blocks, in order.

### Program 8: a problem as a value

When the same problem is solved from several starting points, or handed to another function, bundle it into a `ConstrainedProblem`. You can fill the struct directly or use the `problem()` builder, which accepts the same callables as the struct and picks the right slot. Either value is passed to `optima` with a start given as a plain vector or as a pack.

```cpp
// The objective and constraints can be bundled into one ConstrainedProblem value, either by
// filling the struct directly or with the problem() builder. Both routes describe the same
// problem; the builder is shorter and picks the right objective slot for the callable you give it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto target = optim::trace(vec({1.0, 2.0}), false);
  auto objective = [target](const Pack& p) { return optim::sum(optim::square(p.at("x") - target)); };
  auto budget = [](const Pack& p) { return optim::sum(p.at("x")) - 1.5; };  // x0 + x1 <= 1.5

  // Route 1: fill the struct.
  optim::ConstrainedProblem by_hand;
  by_hand.objective.autodiff = objective;
  by_hand.nonlinear.inequality = budget;

  // Route 2: the builder.
  const optim::ConstrainedProblem built = optim::problem().objective(objective).nl_ineq(budget).build();

  const auto start = vec({0.0, 0.0});
  const auto a = optim::optima(by_hand, start);                           // start as a plain vector
  const auto b = optim::optima(built, optim::pack_series(start));        // start as a named-block pack

  const auto xa = a.params.at("x").to_vec<double>();
  const auto xb = b.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "struct:  x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "builder: x = [" << xb[0] << ", " << xb[1] << "]\n";
  std::cout << "same answer: " << std::boolalpha << (xa == xb) << "\n";
  std::cout << "pack start returns block names: " << b.params.contains("x") << "\n";
  return 0;
}
```

### Output

```text
struct:  x = [0.2500, 1.2500]
builder: x = [0.2500, 1.2500]
same answer: true
pack start returns block names: true
```
Both routes produce the same answer, the point of `x0 + x1 <= 1.5` nearest `(1, 2)`. Starting from a plain vector produces a block named `x`, which is the same result as passing the vector wrapped as a pack.

### Program 9: analytic gradients

If you can write the gradient by hand, you skip automatic differentiation and give the solver a function returning the value and the gradient together. Two forms exist: one over a single vector, and one over named blocks where the gradient is a pack with the same blocks. Declare the callable as a `std::function` of the exact signature so the overload is unambiguous.

```cpp
// When you already have the gradient, supply it directly: the objective returns the value
// together with its gradient and no autodiff is involved. The two-argument form below works
// on a plain vector; the named-block form returns the gradient as a pack with the same blocks.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  // f(x) = (x0 - 1)^2 + 10 (x1 + 2)^2, gradient (2 (x0 - 1), 20 (x1 + 2)).
  std::function<std::pair<pluss::Series, pluss::Series>(const pluss::Series&)> on_vector =
      [&](const pluss::Series& x) {
        const auto v = x.to_vec<double>();
        const double value = (v[0] - 1) * (v[0] - 1) + 10 * (v[1] + 2) * (v[1] + 2);
        return std::pair{pluss::make_series(value).to(f64), vec({2 * (v[0] - 1), 20 * (v[1] + 2)})};
      };
  const auto a = optim::optima(on_vector, vec({0.0, 0.0}));

  std::function<std::pair<pluss::Series, optim::ParameterPack<pluss::Series>>(const optim::ParameterPack<pluss::Series>&)>
      on_blocks = [&](const optim::ParameterPack<pluss::Series>& p) {
        const double u = p.at("u").item<double>();
        const double w = p.at("w").item<double>();
        const double value = (u - 1) * (u - 1) + 10 * (w + 2) * (w + 2);
        return std::pair{pluss::make_series(value).to(f64),
                         optim::pack_scalars({{"u", pluss::make_series(2 * (u - 1)).to(f64)},
                                              {"w", pluss::make_series(20 * (w + 2)).to(f64)}})};
      };
  const auto b = optim::optima(on_blocks, optim::pack_scalars({{"u", pluss::make_series(0.0).to(f64)},
                                                               {"w", pluss::make_series(0.0).to(f64)}}));

  const auto xa = a.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "vector form: x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "block form:  u = " << b.params.at("u").item<double>() << ", w = " << b.params.at("w").item<double>() << "\n";
  return 0;
}
```

### Output

```text
vector form: x = [1.0000, -2.0000]
block form:  u = 1.0000, w = -2.0000
```
Both forms find the minimum `(1, -2)`. The solver trusts the gradient you return, so a wrong gradient gives a wrong answer without any warning. The chapter on checking derivatives shows how to audit one against the automatic result.

### Program 10: reading the result

The returned object answers six questions: where, how good, how feasible, how much work, why it stopped, and what each constraint costs. This program prints one of each for a problem with a linear inequality and bounds.

```cpp
// What optima returns. The result carries the answer, the objective value there, how well every
// constraint is met, how much work was done, why the solver stopped, and the Lagrange multipliers.
// Problem: minimize (x0 - 2)^2 + (x1 - 1)^2 subject to x0 + x1 <= 2 and 0 <= x <= 1.8.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = optim::trace(vec({2.0, 1.0}), false);
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - target)); };
  const auto constraints = optim::constraints()
                               .ln_ineq(mat({1, 1}, 1, 2), vec({2.0}))
                               .lbs(vec({0.0, 0.0}))
                               .ubs(vec({1.8, 1.8}));
  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, constraints);

  std::cout << std::fixed << std::setprecision(4) << std::boolalpha;
  const auto x = r.params.at("x").to_vec<double>();
  std::cout << "answer           x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective value    = " << r.evaluation.value.item<double>() << "\n";
  std::cout << "feasible           = " << r.evaluation.feasible << "\n";
  std::cout << "max violation      = " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "converged          = " << r.converged << "\n";
  std::cout << "exit flag          = " << static_cast<int>(r.exit_flag) << " (Converged is 1)\n";
  std::cout << "iterations         = " << r.iteration << "\n";
  std::cout << "evaluations        = " << r.function_evaluations << "\n";
  std::cout << "stationarity below 1e-5 = " << (r.stationarity < 1e-5) << "\n";
  std::cout << "message            = " << r.message << "\n";

  const auto& m = r.multipliers;
  auto count = [](const pluss::Series& s) { return s.defined() ? s.numel() : 0; };
  std::cout << "linear inequality multiplier = " << m.linear_inequalities.to_vec<double>()[0] << "\n";
  std::cout << "multiplier counts: linear eq " << count(m.linear_equalities) << ", nonlinear eq "
            << count(m.nonlinear_equalities) << ", linear ineq " << count(m.linear_inequalities)
            << ", nonlinear ineq " << count(m.nonlinear_inequalities) << ", lower bounds "
            << count(m.lower_bounds) << ", upper bounds " << count(m.upper_bounds) << "\n";

  std::cout << "-- compact summary --\n";
  std::cout << std::defaultfloat << std::setprecision(6);
  optim::reports::compact_summary(std::cout, r) << "\n";
  return 0;
}
```

### Output

```text
answer           x = [1.5000, 0.5000]
objective value    = 0.5000
feasible           = true
max violation      = 0.0000
converged          = true
exit flag          = 1 (Converged is 1)
iterations         = 3
evaluations        = 4
stationarity below 1e-5 = true
message            = optimality and feasibility tolerances satisfied
linear inequality multiplier = 1.0000
multiplier counts: linear eq 0, nonlinear eq 0, linear ineq 1, nonlinear ineq 0, lower bounds 2, upper bounds 2
-- compact summary --
algorithm=sqp exit=1 iterations=3 evaluations=4 objective=0.5 violation=0 first_order=5.35547e-09 backend_parameter=1.25 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
```
`params` holds the answer, `evaluation` the objective value and feasibility at the answer, `iteration` and `function_evaluations` the work, `exit_flag` and `message` the reason, and `multipliers` how strongly each constraint pushes on the answer. The multiplier of the linear inequality is `1`, the price of tightening it: the objective would improve by about that much per unit the limit is relaxed. The zero counts for equalities show that multipliers exist only for the constraint families the problem uses. `converged` is true exactly when the exit flag is `Converged`.

## Decisions

- **One vector or named blocks?** Use one vector when the unknowns are interchangeable coordinates, such as portfolio weights. Use named blocks when they mean different things or have different shapes, such as a matrix and an offset. Once you use blocks, build bounds as packs with the same names.
- **Automatic or analytic gradient?** Start with `Trace`. Move to an analytic gradient only when you have a reason to, and audit it before trusting it.
- **Inline objective or a `ConstrainedProblem`?** Inline for one-off solves. A `ConstrainedProblem` when the problem is reused, stored, or passed around.
- **Struct or builder?** The builder is shorter and chooses the right objective slot for the callable you give it. Fill the struct when you need slots the builder does not cover.

## Pitfalls

### Bounds that do not match the blocks

`lbs` and `ubs` with a plain vector attach to a block named `x`. On a problem whose blocks have other names, `optima` refuses the problem before it starts and shows both layouts, so you can see the mismatch at once. The fix is to build bounds as a pack with the same blocks.

```cpp
// Pitfall: bounds given as a plain vector are attached to a block named "x". On a problem whose
// unknowns have other names, they do not line up, and optima refuses the problem before it
// starts, naming both layouts. The fix is to build the bounds as a pack with the same blocks.
#include <pluss/ma.h>

#include <iostream>
#include <string>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  using Pack = optim::ParameterPack<optim::Trace>;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };

  auto objective = [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")); };
  const auto start = optim::pack_scalars({{"a", scalar(1.0)}, {"b", scalar(1.0)}});

  try {
    const auto wrong = optim::constraints().lbs(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64));
    (void)optim::optima(objective, start, {}, wrong);
  } catch (const std::exception& e) {
    std::string message = e.what();
    std::cout << "refused: " << message.substr(0, message.find('\n')) << "\n";
  }

  const auto right = optim::constraints().lbs(optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.0)}}));
  const auto result = optim::optima(objective, start, {}, right);
  std::cout << "with matching blocks: a = " << result.params.at("a").item<double>() << ", b = "
            << result.params.at("b").item<double>() << "\n";
  return 0;
}
```

### Output

```text
refused: optim::evaluate_problem: lower bounds must have the same named blocks and shapes as the parameters (parameters: a[1], b[1]; lower bounds: x[2])
with matching blocks: a = 0.5, b = 1.20218e-10
```
The first call is refused with a message naming the parameter blocks and the bounds layout. The second succeeds once the bounds carry the block names `a` and `b`.

### An objective that returns more than one number

The objective must reduce to a single number. Returning a vector of residuals, instead of their sum of squares, is the usual slip. The solver returns immediately with the `InvalidProblem` flag and a message that names the fix.

```cpp
// Pitfall: the objective must return one number. Returning a vector of residuals instead of
// their sum of squares is a common slip. optima does not throw; it returns immediately with the
// InvalidProblem exit flag and a message that says what to change.
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::optim;
  const auto start = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(pluss::ScalarType::Float64);

  const auto result = optim::optima([](const optim::Trace& x) { return optim::square(x); }, start);

  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::string message = result.message;
  std::cout << "message: " << message.substr(0, message.find('\n')) << "\n";
  return 0;
}
```

### Output

```text
exit flag: -5
converged: false
message: objective autodiff callback failed: optim: the objective must return a single number, but it returned 2 values; reduce it with sum() or mean()
```
`optima` does not throw here: it returns a result with exit flag `-5` and an explanation, so a calling program can inspect it. `converged` is false because the exit flag is not `Converged`.

## Where next

The next chapter reads results in depth: every exit flag, what each field of the output means, how to watch a run with progress hooks, and how to tell a stalled run from a converged one.
