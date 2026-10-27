<!-- generated from bounds_equalities_and_inequalities.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Bounds, equalities and inequalities

[Back to Septima 2: Constraints](../septima_02_constraints.md)

Septima handles constraints with an augmented-Lagrangian scheme: bounds by projection, and equalities and inequalities by a penalty plus a multiplier that is updated as the run proceeds. The penalty starts at a value you choose and is raised when the violation is not falling fast enough; the multiplier converges to the Lagrange multiplier of the constraint, which is the constraint's price and is returned in the result's state.

The four programs go in order of difficulty: a box on every coordinate (the answer clips the target into the box), a single linear equality, a nonlinear inequality that keeps the answer inside a disc, and finally a problem with one of every kind of constraint, coupled so that it is non-convex, solved by both Septima solvers and by Optima as the reference. The last comparison is the honest answer to "which tool?": the answers agree, and the difference is the amount of work.

## Program 1: Bounds

The target is `(2, 1, -1)` and the objective is half the squared distance to it, with the box `[0, 1.5]` on every coordinate. The answer clips each coordinate of the target into the box: `(1.5, 1, 0)`. The run starts outside the box, at `(3, -2, 0.5)`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0, -1.0});
  auto objective = [&](const pluss::Series& x) -> pluss::Series { return (x - target).square().sum() * 0.5; };

  optim::SeptimaConstraints box;
  box.lbs = {vec({0.0, 0.0, 0.0})};
  box.ubs = {vec({1.5, 1.5, 1.5})};

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(11) << "exit" << std::setw(7) << "iters" << std::setw(24) << "x" << std::setw(11)
            << "gradient" << "stationarity\n";
  for (const auto optimizer : {optim::SeptimaOptimizer::Adam, optim::SeptimaOptimizer::Lbfgs}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optimizer;
    options.step.adam.learning_rate = 0.05;
    options.max_iterations = 2000;
    // Start outside the box: the first step already brings it back in.
    const auto r = optim::septima(objective, vec({3.0, -2.0, 0.5}), options, box);
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>() << ", "
      << r.params[0].select(0, 2).item<double>();
    std::cout << std::left << std::setw(10) << (optimizer == optim::SeptimaOptimizer::Adam ? "Adam" : "L-BFGS") << std::setw(11)
              << (r.converged ? "Converged" : "not converged") << std::setw(7) << r.iteration << std::setw(24) << x.str() << std::scientific
              << std::setprecision(1) << std::setw(11) << r.gradient_inf_norm << r.stationarity << "\n";
  }
  return 0;
}
```

Output

```text
optimizer exit       iters  x                       gradient   stationarity
Adam      Converged  292    1.5000, 1.0000, 0.0000  1.0e+00    2.8e-07
L-BFGS    Converged  2      1.5000, 1.0000, 0.0000  1.0e+00    0.0e+00
```
Both optimizers find `(1.5, 1, 0)`, Adam in 292 iterations and L-BFGS in 2. Look at the last two columns. The objective's own gradient is 1.0 at the answer, because the objective still pulls toward the target through the bounds on the first and third coordinates. The stationarity is 2.8e-07 for Adam and exactly 0 for L-BFGS, because the components that point out of the box are removed. This is why a run with active bounds converges, and why `gradient_inf_norm` alone would be the wrong thing to watch. Because every iterate is clipped, a bound never causes a violation, and the start outside the box is brought inside by the first step.

## Program 2: An equality constraint

Be as close as possible to `(2, 1)` while the coordinates sum to 2: minimize `0.5 |x - (2, 1)|^2` subject to `x0 + x1 - 2 = 0`. The answer is `(1.5, 0.5)`, and the multiplier is 0.5, because the objective's gradient there is `(-0.5, -0.5)`, which is 0.5 times the gradient of the constraint. The table runs both optimizers with the defaults at three starting penalties, and then with the multiplier step overridden. `updates` counts the multiplier updates and `penalty x` is the factor by which the run raised the penalty.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) -> pluss::Series { return (x - target).square().sum() * 0.5; };
  optim::SeptimaConstraints constraint;
  constraint.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 2.0}; };

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(9) << "penalty" << std::setw(11) << "dual step" << std::setw(15) << "exit" << std::setw(7) << "iters"
            << std::setw(9) << "updates" << std::setw(10) << "penalty x" << std::setw(12) << "violation" << std::setw(12) << "multiplier" << "x\n";
  struct Row { bool lbfgs; double penalty; double dual; };
  const Row rows[] = {{false, 1, 0}, {false, 10, 0}, {false, 100, 0}, {true, 1, 0}, {true, 10, 0}, {true, 100, 0}, {false, 10, 0.1}, {true, 10, 0.1}};
  for (const auto& row : rows) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 0.1;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.equality_penalty = row.penalty;
    options.step.dual_learning_rate = row.dual;  // 0 = the library default, the penalty itself
    const auto r = optim::septima(objective, vec({0.0, 0.0}), options, constraint);
    std::ostringstream x, dual;
    x << std::fixed << std::setprecision(3) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    if (row.dual > 0.0) dual << row.dual; else dual << "default";
    std::cout << std::left << std::setw(10) << (row.lbfgs ? "L-BFGS" : "Adam") << std::setw(9) << std::defaultfloat << row.penalty << std::setw(11) << dual.str()
              << std::setw(15) << (r.converged ? "Converged" : "MaxIterations") << std::setw(7) << r.iteration << std::setw(9) << r.state.dual_updates << std::setw(10)
              << r.state.penalty_scale << std::scientific << std::setprecision(1) << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(3)
              << std::setw(12) << r.state.lambda_equalities[0].item<double>() << x.str() << "\n";
  }
  return 0;
}
```

Output

```text
optimizer penalty  dual step  exit           iters  updates  penalty x violation   multiplier  x
Adam      1        default    Converged      696    15       100       3.8e-07     0.500       1.500, 0.500
Adam      10       default    Converged      347    6        1         6.2e-07     0.500       1.500, 0.500
Adam      100      default    Converged      1392   27       1         5.8e-11     0.500       1.500, 0.500
L-BFGS    1        default    Converged      16     6        10        5.7e-07     0.500       1.500, 0.500
L-BFGS    10       default    Converged      15     5        1         2.4e-07     0.500       1.500, 0.500
L-BFGS    100      default    Converged      10     3        1         1.2e-07     0.500       1.500, 0.500
Adam      10       0.1        MaxIterations  3000   61       1e+03     4.8e-05     0.020       1.500, 0.500
L-BFGS    10       0.1        Converged      31     11       1e+05     4.9e-07     0.011       1.500, 0.500
```
With the defaults every row converges to `(1.500, 0.500)` with a multiplier of exactly 0.500, whatever the starting penalty. L-BFGS needs 16, 15 and 10 iterations for penalties of 1, 10 and 100; Adam needs 696, 347 and 1392, because each step is bounded by its learning rate. The penalty column shows the schedule at work: from a penalty of 1 the run raised it by a factor of 10 for L-BFGS and 100 for Adam, because the violation did not shrink fast enough; from 10 and 100 it left the penalty alone. A penalty that is too small is repaired, and one that is generous costs nothing in accuracy, only some iterations for Adam.

The last two rows override the multiplier step with 0.1, far below the penalty of 10. The schedule then compensates in the only way left: it inflates the penalty (by a factor of a thousand for Adam and a hundred thousand for L-BFGS) to force the violation down, and the multiplier, which now learns very slowly, ends at 0.020 and 0.011 instead of 0.5. The point is still right, but the multiplier is not. Leave `dual_learning_rate` at its default of 0, which means "use the penalty", unless you have a reason.

## Program 3: An inequality constraint

The target `(2, 1)` lies outside the unit disc, and the problem is to be as close to it as possible while staying inside: minimize `0.5 |x - (2, 1)|^2` subject to `x0^2 + x1^2 - 1 <= 0`. The answer is the point of the circle in the direction of the target, `(2, 1) / sqrt(5) = (0.8944, 0.4472)`, and the multiplier is `(sqrt(5) - 1) / 2 = 0.618`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) -> pluss::Series { return (x - target).square().sum() * 0.5; };
  optim::SeptimaConstraints disc;
  disc.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 1.0}; };

  std::cout << "exact answer (" << std::fixed << std::setprecision(4) << 2.0 / std::sqrt(5.0) << ", " << 1.0 / std::sqrt(5.0)
            << "), multiplier " << (std::sqrt(5.0) - 1.0) / 2.0 << "\n\n";
  std::cout << std::left << std::setw(34) << "run" << std::setw(8) << "iters" << std::setw(20) << "x" << std::setw(12) << "violation" << "multiplier\n";
  const auto row = [](const char* label, const optim::SeptimaResult& r) {
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    std::cout << std::left << std::setw(34) << label << std::setw(8) << r.iteration << std::setw(20) << x.str() << std::scientific << std::setprecision(1)
              << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(4) << r.state.lambda_inequalities[0].item<double>() << "\n";
  };

  optim::SeptimaSolveOptions base;  // the library defaults for the constraint schedule
  base.max_iterations = 4000;
  base.gradient_tolerance = 1e-5;
  base.step_tolerance = 1e-5;
  base.step.adam.learning_rate = 0.1;

  row("Adam from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), base, disc));
  row("Adam from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), base, disc));

  optim::SeptimaSolveOptions lbfgs = base;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  row("L-BFGS from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), lbfgs, disc));
  row("L-BFGS from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  lbfgs.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  row("L-BFGS, strong Wolfe, far start", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  return 0;
}
```

Output

```text
exact answer (0.8944, 0.4472), multiplier 0.6180

run                               iters   x                   violation   multiplier
Adam from (0, 0)                  295     0.8944, 0.4472      0.0e+00     0.6180
Adam from (-2, 1.5)               660     0.8944, 0.4472      0.0e+00     0.6180
L-BFGS from (0, 0)                19      0.8944, 0.4472      4.4e-07     0.6180
L-BFGS from (-2, 1.5)             24      0.8944, 0.4472      4.4e-07     0.6180
L-BFGS, strong Wolfe, far start   24      0.8944, 0.4472      4.4e-07     0.6180
```
Every run finds the exact answer and the exact multiplier, 0.6180, from a start inside the disc and from one far outside it. Adam needs 295 and 660 iterations; L-BFGS needs 19 and 24. From the far start the multiplier first grows because the constraint is badly violated, and then has to come back down to 0.618 as the iterate returns; the multiplier update lets it do both. With constraints present, L-BFGS always protects each step with a backtracking test on the augmented function, because the inequality term has a kink where the constraint becomes active, so the line-search setting makes no difference in the last two rows: they are identical.

## Program 4: The same problem in both solvers

Three parameters and one of every kind of constraint: bounds, a linear and a nonlinear equality, a linear and a nonlinear inequality. The coupling `a b = 0.3` makes the problem non-convex. Optima, which solves a quadratic program at every step, is the reference.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> objective = [](const Pack& p) {
    return p.at("a").square() + 2.0 * p.at("b").square() + 3.0 * p.at("c").square() - 4.0 * p.at("a");
  };
  const auto start = optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.5)}, {"c", scalar(0.5)}});

  std::cout << std::left << std::setw(26) << "solver" << std::setw(16) << "exit" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << std::setw(12) << "violation" << "a, b, c\n";
  const auto row = [](const char* label, const char* exit, std::int64_t evals, double value, double violation, double a, double b, double c) {
    std::cout << std::left << std::setw(26) << label << std::setw(16) << exit << std::setw(8) << evals << std::fixed << std::setprecision(5)
              << std::setw(12) << value << std::scientific << std::setprecision(1) << std::setw(12) << violation << std::fixed << std::setprecision(4) << a
              << ", " << b << ", " << c << "\n";
  };

  // Optima: constraints as the families it knows.
  const auto optima_constraints =
      optim::constraints()
          .lbs(optim::pack_scalars({{"a", scalar(0)}, {"b", scalar(0)}, {"c", scalar(0)}}))
          .ubs(optim::pack_scalars({{"a", scalar(1.5)}, {"b", scalar(2)}, {"c", scalar(2)}}))
          .ln_eq(pluss::make_series(std::vector<double>{0, 1, 1}, {1, 3}).to(f64), pluss::make_series(std::vector<double>{1.0}, {1}).to(f64))
          .ln_ineq(pluss::make_series(std::vector<double>{1, 1, 1}, {1, 3}).to(f64), pluss::make_series(std::vector<double>{2.5}, {1}).to(f64))
          .nl_eq([](const Pack& p) { return p.at("a") * p.at("b") - 0.3; })
          .nl_ineq([](const Pack& p) { return p.at("c").square() - 0.6; });
  const auto o = optim::optima(objective, start, {}, optima_constraints);
  row("Optima", "converged", o.function_evaluations, o.evaluation.value.item<double>(), o.evaluation.max_constraint_violation,
      as_f64(o.params.at("a")), as_f64(o.params.at("b")), as_f64(o.params.at("c")));

  // Septima: the same requirements as bounds and residual functions over the list of blocks.
  optim::SeptimaConstraints c;
  c.lbs = {scalar(0.0), scalar(0.0), scalar(0.0)};
  c.ubs = {scalar(1.5), scalar(2.0), scalar(2.0)};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[1] + x[2] - 1.0, x[0] * x[1] - 0.3}; };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0] + x[1] + x[2] - 2.5, x[2].square() - 0.6}; };

  const auto name = [](const optim::SeptimaResult& r) {
    switch (r.exit_flag) {
      case optim::SeptimaExitFlag::Converged: return "converged";
      case optim::SeptimaExitFlag::MaxIterations: return "MaxIterations";
      case optim::SeptimaExitFlag::NoProgress: return "NoProgress";
      default: return "other";
    }
  };
  optim::SeptimaSolveOptions adam;  // the library's constraint schedule, default penalties
  adam.max_iterations = 5000;
  adam.gradient_tolerance = 1e-5;
  adam.step_tolerance = 1e-6;
  adam.step.adam.learning_rate = 0.02;
  const auto a = optim::septima(objective, start, adam, c);
  row("Septima, Adam", name(a), a.function_evaluations, a.objective_value, a.constraint_violation, as_f64(a.params[0]), as_f64(a.params[1]),
      as_f64(a.params[2]));

  optim::SeptimaSolveOptions lbfgs = adam;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto l = optim::septima(objective, start, lbfgs, c);
  row("Septima, L-BFGS", name(l), l.function_evaluations, l.objective_value, l.constraint_violation, as_f64(l.params[0]), as_f64(l.params[1]),
      as_f64(l.params[2]));
  return 0;
}
```

Output

```text
solver                    exit            evals   objective   violation   a, b, c
Optima                    converged       6       -1.65076    2.2e-10     1.3309, 0.2254, 0.7746
Septima, Adam             converged       854     -1.65075    8.3e-08     1.3309, 0.2254, 0.7746
Septima, L-BFGS           converged       242     -1.65076    5.0e-07     1.3310, 0.2254, 0.7746
```
Septima reaches the same solution as Optima, `(1.331, 0.2254, 0.7746)` with an objective of -1.65076, with the default settings. Adam needs 854 evaluations and ends with a violation of 8.3e-08; L-BFGS needs 242 evaluations and ends with 5.0e-07. Optima needs 6 evaluations and ends with 2.2e-10. So the answers agree, and the differences are in cost and in precision: Septima stops when the violation is within its tolerance (1e-6 by default), Optima solves each step to a far tighter one, and Optima uses second-order information that lets it finish in a handful of steps.

Choose by what the problem needs. For a small problem with a few nonlinear constraints that must hold to high precision, Optima is the better tool. For a problem with many parameters, mostly bounds and simple constraints, or complex parameters, or an objective you can differentiate but whose Hessian is out of reach, Septima gives the same answer with only gradients. Chapter 5 gives a decision table.

## Decisions and pitfalls

- **At a bound the objective's gradient is not zero.** The objective still pulls toward the target through an active bound; the stationarity (with blocked components removed) is the number that converges.
- **The multiplier is the price of the constraint.** It converges to the exact value whatever the starting penalty; check it against the analytic value on problems where you can compute one.
- **Do not shrink the multiplier step far below the penalty.** The schedule then inflates the penalty to force the violation down, the multiplier learns slowly, and the run costs far more.
- **Adam needs hundreds of iterations where L-BFGS needs tens.** For constrained problems prefer L-BFGS unless you need Adam's behaviour on noisy objectives.
- **For a small problem needing high precision, use Optima.** Septima reaches the same solution with many more evaluations and a violation of order 1e-7; its strength is size, complex parameters and objectives whose Hessian is out of reach.

Where next: [Chapter 3](../septima_03_tuning_and_robustness.md) tunes the solvers.
