<!-- generated from blocks_values_and_reading_the_result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Blocks, problems as values, gradients and reading the result

[Back to Optima 1: Your First Constrained Problems](../optima_01_first_problems.md)

The first page treated the unknowns as a single anonymous vector. Real problems have unknowns that *mean* something: the slope and the intercept of a line, the weights and the offset of a model. `optima` lets you name them. A problem with several quantities of different shapes uses one **block** per quantity, and blocks keep their names and shapes in the answer, so results are read back by name rather than by position.

This page also covers the other ways to hand a problem over: bundled as a value (a `ConstrainedProblem`) when it is solved repeatedly or passed to another function, or with a hand-written gradient when you have one. It then reads the result object, which answers six questions (where, how good, how feasible, how much work, why it stopped, and what each constraint costs), and ends with the two slips that account for most first-day errors: bounds that do not match the block names, and an objective that returns more than one number.

## Program 7: Named blocks and a model

Named blocks shine when the unknowns mean something. This program fits a straight line to five points. The objective reads like the model, with `slope` and `intercept` as separate blocks, and the answer is read back by those names.

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
  using Pack = optim::ParameterPack<pluss::Series>;

  const auto t = vec({0.0, 1.0, 2.0, 3.0, 4.0});
  const auto y = vec({1.1, 2.9, 5.2, 7.1, 8.8});

  auto sum_squared_error = [&](const Pack& p) {
    const auto prediction = p.at("slope") * t + p.at("intercept");
    return (prediction - y).square().sum();
  };

  const auto start = optim::pack_scalars({{"slope", pluss::make_series(0.0).to(f64)},
                                          {"intercept", pluss::make_series(0.0).to(f64)}});
  const auto result = optim::optima(sum_squared_error, start);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "slope     = " << as_f64(result.params.at("slope")) << "\n";
  std::cout << "intercept = " << as_f64(result.params.at("intercept")) << "\n";
  std::cout << "blocks returned: " << result.params.size() << "\n";
  return 0;
}
```

Output

```text
slope     = 1.9600
intercept = 1.1000
blocks returned: 2
```
The slope `1.9601` and intercept `1.0999` match the closed-form least-squares line for these data, `1.96` and `1.10`, to the solver tolerance. The result keeps both blocks, in order.

## Program 8: A problem as a value

When the same problem is solved from several starting points, or handed to another function, bundle it into a `ConstrainedProblem`. You can fill the struct directly or use the `problem()` builder, which accepts the same callables as the struct and picks the right slot. Either value is passed to `optima` with a start given as a plain vector or as a pack.

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
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto target = vec({1.0, 2.0});
  auto objective = [target](const Pack& p) { return (p.at("x") - target).square().sum(); };
  auto budget = [](const Pack& p) { return p.at("x").sum() - 1.5; };  // x0 + x1 <= 1.5

  // Route 1: fill the struct.
  optim::ConstrainedProblem by_hand;
  by_hand.objective.autodiff = objective;
  by_hand.nonlinear.inequality = budget;

  // Route 2: the builder.
  const optim::ConstrainedProblem built = optim::problem().objective(objective).nl_ineq(budget).build();

  const auto start = vec({0.0, 0.0});
  const auto a = optim::optima(by_hand, start);                           // start as a plain vector
  const auto b = optim::optima(built, optim::pack_series(start));        // start as a named-block pack

  const auto xa = a.params.at("x").host<double>();
  const auto xb = b.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "struct:  x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "builder: x = [" << xb[0] << ", " << xb[1] << "]\n";
  std::cout << "same answer: " << std::boolalpha << (xa == xb) << "\n";
  std::cout << "pack start returns block names: " << b.params.contains("x") << "\n";
  return 0;
}
```

Output

```text
struct:  x = [0.2500, 1.2500]
builder: x = [0.2500, 1.2500]
same answer: true
pack start returns block names: true
```
Both routes produce the same answer, the point of `x0 + x1 <= 1.5` nearest `(1, 2)`. Starting from a plain vector produces a block named `x`, which is the same result as passing the vector wrapped as a pack.

## Program 9: Analytic gradients

If you can write the gradient by hand, you skip automatic differentiation and give the solver a function returning the value and the gradient together. Two forms exist: one over a single vector, and one over named blocks where the gradient is a pack with the same blocks. Declare the callable as a `std::function` of the exact signature so the overload is unambiguous.

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

  // f(x) = (x0 - 1)^2 + 10 (x1 + 2)^2, gradient (2 (x0 - 1), 20 (x1 + 2)).
  std::function<std::pair<pluss::Series, pluss::Series>(const pluss::Series&)> on_vector =
      [&](const pluss::Series& x) {
        const auto v = x.host<double>();
        const double value = (v[0] - 1) * (v[0] - 1) + 10 * (v[1] + 2) * (v[1] + 2);
        return std::pair{pluss::make_series(value).to(f64), vec({2 * (v[0] - 1), 20 * (v[1] + 2)})};
      };
  const auto a = optim::optima(on_vector, vec({0.0, 0.0}));

  std::function<std::pair<pluss::Series, optim::ParameterPack<pluss::Series>>(const optim::ParameterPack<pluss::Series>&)>
      on_blocks = [&](const optim::ParameterPack<pluss::Series>& p) {
        const double u = as_f64(p.at("u"));
        const double w = as_f64(p.at("w"));
        const double value = (u - 1) * (u - 1) + 10 * (w + 2) * (w + 2);
        return std::pair{pluss::make_series(value).to(f64),
                         optim::pack_scalars({{"u", pluss::make_series(2 * (u - 1)).to(f64)},
                                              {"w", pluss::make_series(20 * (w + 2)).to(f64)}})};
      };
  const auto b = optim::optima(on_blocks, optim::pack_scalars({{"u", pluss::make_series(0.0).to(f64)},
                                                               {"w", pluss::make_series(0.0).to(f64)}}));

  const auto xa = a.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "vector form: x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "block form:  u = " << as_f64(b.params.at("u")) << ", w = " << as_f64(b.params.at("w")) << "\n";
  return 0;
}
```

Output

```text
vector form: x = [1.0000, -2.0000]
block form:  u = 1.0000, w = -2.0000
```
Both forms find the minimum `(1, -2)`. The solver trusts the gradient you return, so a wrong gradient gives a wrong answer without any warning. The chapter on checking derivatives shows how to audit one against the automatic result.

## Program 10: Reading the result

The returned object answers six questions: where, how good, how feasible, how much work, why it stopped, and what each constraint costs. This program prints one of each for a problem with a linear inequality and bounds.

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

  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };
  const auto constraints = optim::constraints()
                               .ln_ineq(mat({1, 1}, 1, 2), vec({2.0}))
                               .lbs(vec({0.0, 0.0}))
                               .ubs(vec({1.8, 1.8}));
  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, constraints);

  std::cout << std::fixed << std::setprecision(4) << std::boolalpha;
  const auto x = r.params.at("x").host<double>();
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
  std::cout << "linear inequality multiplier = " << m.linear_inequalities.host<double>()[0] << "\n";
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

Output

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

## Program 11: Bounds that do not match the blocks

`lbs` and `ubs` with a plain vector attach to a block named `x`. On a problem whose blocks have other names, `optima` refuses the problem before it starts and shows both layouts, so you can see the mismatch at once. The fix is to build bounds as a pack with the same blocks.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  using Pack = optim::ParameterPack<pluss::Series>;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };

  auto objective = [](const Pack& p) { return p.at("a").square() + p.at("b").square(); };
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
  std::cout << "with matching blocks: a = " << as_f64(result.params.at("a")) << ", b = "
            << as_f64(result.params.at("b")) << "\n";
  return 0;
}
```

Output

```text
refused: optim::evaluate_problem: lower bounds must have the same named blocks and shapes as the parameters (parameters: a[1], b[1]; lower bounds: x[2])
with matching blocks: a = 0.5, b = 1.20218e-10
```
The first call is refused with a message naming the parameter blocks and the bounds layout. The second succeeds once the bounds carry the block names `a` and `b`.

## Program 12: An objective that returns more than one number

The objective must reduce to a single number. Returning a vector of residuals, instead of their sum of squares, is the usual slip. The solver returns immediately with the `InvalidProblem` flag and a message that names the fix.

```cpp
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto start = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(pluss::ScalarType::Float64);

  const auto result = optim::optima([](const pluss::Series& x) { return x.square(); }, start);

  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::string message = result.message;
  std::cout << "message: " << message.substr(0, message.find('\n')) << "\n";
  return 0;
}
```

Output

```text
exit flag: -5
converged: false
message: objective autodiff callback failed: optim: the objective must return a single number, but it returned 2 values; reduce it with sum() or mean()
```
`optima` does not throw here: it returns a result with exit flag `-5` and an explanation, so a calling program can inspect it. `converged` is false because the exit flag is not `Converged`.

## Decisions and pitfalls

- **One vector or named blocks?** Use one vector when the unknowns are interchangeable coordinates (portfolio weights); use named blocks when they mean different things or have different shapes. Once you use blocks, build bounds as packs with the same block names.
- **Automatic or analytic gradient?** Start with plain `Series` code. Move to a hand-written gradient only when you have a reason, and audit it first: the solver trusts the gradient you give it, and a wrong one gives a wrong answer without a warning.
- **Inline objective or a `ConstrainedProblem`?** Inline for a one-off solve; a `ConstrainedProblem` when the problem is reused, stored or passed around.
- **A returned vector is not an objective.** The objective must reduce to one number; return the sum of squares, not the residual vector. `optima` reports `InvalidProblem` instead of throwing, so a calling program can inspect the result.
- **Read the flag, not just the point.** `converged` is true exactly when the exit flag is `Converged`; the next chapter covers what every other flag means.

Where next: [Chapter 2](../optima_02_reading_results.md) reads results in depth: every exit flag, every field of the report, and how to watch a run.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stating_a_constrained_problem.html">
    <span class="label">Previous</span>
    <span class="title">Stating a constrained problem</span>
  </a>
  <a class="next-link" href="complex_parameters.html">
    <span class="label">Next</span>
    <span class="title">Complex parameters</span>
  </a>
</div>
<!-- apiref:nav:end -->
