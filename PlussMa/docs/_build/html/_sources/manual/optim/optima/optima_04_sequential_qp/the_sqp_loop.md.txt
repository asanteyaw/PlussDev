<!-- generated from the_sqp_loop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The sequential QP loop

[Back to Optima 4: Sequential QP in Depth](../optima_04_sequential_qp.md)

Sequential quadratic programming (SQP) is `optima`'s default algorithm, and it is worth understanding because most of what you will tune lives in it. Each iteration replaces the problem by a quadratic model of the objective with *linearised* constraints, solves that subproblem for a step, and then decides how much of the step to take. The quantity that decides is a **merit function**: the objective plus a weight (the *penalty*) times the constraint violation. The line search accepts a step only if it reduces the merit, so the penalty weight is the exchange rate between "better objective" and "closer to feasible".

The four programs follow the loop from the outside in. The first prints it iteration by iteration (the step, the penalty, the stationarity and the violation); the second varies the starting penalty weight; the third varies each line-search setting on its own; and the fourth shows the *second-order correction*, the repair for the case where the linearised constraints mislead the line search near a curved constraint (the Maratos effect).

## Program 1: Watching the loop

The progress hook receives one snapshot per iteration. The program prints the step taken, the penalty weight, the stationarity measure and the constraint violation on a four-variable problem with one nonlinear inequality and one nonlinear equality.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::SequentialQP;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "step" << std::setw(12) << "penalty"
            << std::setw(14) << "stationarity" << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.step_size
              << std::setw(12) << s.backend_parameter << std::setw(14) << s.stationarity << std::setw(12)
              << s.evaluation.max_constraint_violation << std::fixed << std::setprecision(5) << std::setw(12)
              << s.evaluation.value.item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints());
  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations, "
            << r.function_evaluations << " evaluations\n" << r.message << "\n";
  return 0;
}
```

Output

```text
 iter        step     penalty  stationarity   violation  objective
    1    1.00e+00    5.00e+00      9.37e-02    1.39e+00    16.06250
    2    1.00e+00    2.50e+00      3.32e-02    8.08e-02    16.96396
    3    1.00e+00    1.25e+00      3.36e-03    4.61e-04    17.01372
    4    1.00e+00    1.21e+00      1.84e-05    8.24e-08    17.01402
    4    1.00e+00    1.21e+00      4.49e-07    8.24e-08    17.01402

exit flag 1, 4 iterations, 5 evaluations
optimality and feasibility tolerances satisfied
```
Read the columns together. The violation falls from 1.39 to 8.2e-08 in four iterations while the objective settles at 17.01402. The penalty weight starts at 5.00 and falls to 1.21: the solver sets it from the size of the multipliers rather than keeping the starting guess. The full step (`1.00`) is accepted every time, which is the sign of a model that predicts the problem well. The last iteration appears twice, with stationarity 1.84e-05 and then 4.49e-07: the second line is the same point after the multiplier estimate has been refreshed, and it is that value which satisfies the tolerance.

## Program 2: The penalty weight

A weight that is too small lets iterates drift off the constraints; a weight that is too large makes every step look like a loss of feasibility. The program starts from a curved valley (Rosenbrock's function with an inequality and an equality constraint) at an infeasible point, with `initial_penalty` from 0.1 to 1000.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> start_point() { return optim::pack_scalars({{"x", scalar(-1.9)}, {"y", scalar(2.0)}}); }
// Rosenbrock's curved valley, restricted to the disc x^2 + y^2 <= 1.5 and to the hyperbola x y = 0.4.
auto objective = [](const Pack& p) {
  return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
};
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return p.at("x").square() + p.at("y").square() - 1.5; })
      .nl_eq([](const Pack& p) { return p.at("x") * p.at("y") - 0.4; });
}

int main() {
  std::cout << std::left << std::setw(34) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(13) << "objective" << "violation\n";
  auto run = [&](const std::string& label, const optim::OptimaOptions& options, const auto& start, const auto& cons, const auto& f) {
    const auto r = optim::optima(f, start, options, cons);
    std::cout << std::left << std::setw(34) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(5)
              << std::setw(13) << r.evaluation.value.template item<double>() << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  };
  for (double w : {0.1, 1.0, 10.0, 100.0, 1000.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_penalty = w;
    run("initial_penalty " + std::to_string(w).substr(0, 7), o, start_point(), constraints(), objective);
  }
  return 0;
}
```

Output

```text
setting                           flag  iters  evals  objective    violation
initial_penalty 0.10000           1     9      14     0.06913      3.5e-07
initial_penalty 1.00000           1     9      14     0.06913      3.5e-07
initial_penalty 10.0000           1     10     14     0.06913      8.8e-10
initial_penalty 100.000           1     10     22     0.06913      5.0e-10
initial_penalty 1000.00           1     12     34     0.06913      1.1e-09
```
All five rows reach the same objective, 0.06913, and the three smallest weights cost the same 14 evaluations. A weight of 100 costs 22 and a weight of 1000 costs 34: a weight that is too large makes every step look like a loss of feasibility, so the line search backtracks more before it accepts one. The solver adapts the weight itself, so a poor starting guess is corrected within a few iterations and costs evaluations, not the answer. Leave the default; if a run fails, look at the line search before the penalty.

## Program 3: The line search

The same problem, with each line-search setting varied on its own.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> start_point() { return optim::pack_scalars({{"x", scalar(-1.9)}, {"y", scalar(2.0)}}); }
// Rosenbrock's curved valley, restricted to the disc x^2 + y^2 <= 1.5 and to the hyperbola x y = 0.4.
auto objective = [](const Pack& p) {
  return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
};
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return p.at("x").square() + p.at("y").square() - 1.5; })
      .nl_eq([](const Pack& p) { return p.at("x") * p.at("y") - 0.4; });
}

int main() {
  std::cout << std::left << std::setw(34) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(13) << "objective" << "violation\n";
  auto run = [&](const std::string& label, const optim::OptimaOptions& options, const auto& start, const auto& cons, const auto& f) {
    const auto r = optim::optima(f, start, options, cons);
    std::cout << std::left << std::setw(34) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(5)
              << std::setw(13) << r.evaluation.value.template item<double>() << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  };
  for (double b : {0.2, 0.5, 0.8}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.backtracking = b;
    run("backtracking " + std::to_string(b).substr(0, 4), o, start_point(), constraints(), objective);
  }
  for (double a : {1e-6, 1e-4, 1e-1}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.armijo = a;
    std::ostringstream label;
    label << "armijo " << a;
    run(label.str(), o, start_point(), constraints(), objective);
  }
  for (std::int64_t k : {1, 3, 20}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_line_search_iterations = k;
    run("max_line_search_iterations " + std::to_string(k), o, start_point(), constraints(), objective);
  }
  for (double s0 : {0.1, 0.5, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_step_size = s0;
    run("initial_step_size " + std::to_string(s0).substr(0, 4), o, start_point(), constraints(), objective);
  }
  return 0;
}
```

Output

```text
setting                           flag  iters  evals  objective    violation
backtracking 0.20                 1     11     18     0.06913      3.8e-07
backtracking 0.50                 1     10     14     0.06913      8.8e-10
backtracking 0.80                 1     10     17     0.06913      5.8e-10
armijo 1e-06                      1     10     14     0.06913      8.8e-10
armijo 0.0001                     1     10     14     0.06913      8.8e-10
armijo 0.1                        1     11     21     0.06913      3.3e-09
max_line_search_iterations 1      1     11     16     0.06913      1.3e-09
max_line_search_iterations 3      1     10     14     0.06913      8.8e-10
max_line_search_iterations 20     1     10     14     0.06913      8.8e-10
initial_step_size 0.10            0     100    101    164.54577    4.0e-01
initial_step_size 0.50            -6    55     149    8.14438      7.5e-13
initial_step_size 1.00            1     10     14     0.06913      8.8e-10
```
Three facts stand out. The default backtracking factor of 0.5 is the cheapest here, 14 evaluations against 17 for 0.8 and 18 for 0.2. A loose `armijo` of 0.1 demands so much decrease that 21 evaluations are spent, while 1e-6 and 1e-4 behave exactly like the default. The trial-step budget barely matters on this problem: one trial step per iteration still converges (11 iterations, 16 evaluations), and so do 3 and 20 (10 iterations, 14 evaluations); Program 5 shows a problem where a budget of one fails. The setting that decides whether the run works at all is `initial_step_size`: 0.1 leaves the run unconverged after 100 iterations at objective 164.546, and 0.5 stops with flag `-6` at 8.14438 after 55 iterations. Keep the full initial step: a Newton-type method gets its fast local convergence from it.

## Program 4: Curved constraints and the second-order correction

Near the solution of a curved equality constraint, the best step of the linearized model can raise both the objective and the violation of the true problem, so a good step is rejected (the Maratos effect). A second-order correction re-solves the subproblem with the constraint measured at the trial point, which cancels the curvature error.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

int main() {
  const auto start = optim::pack_scalars({{"x", scalar(std::cos(0.9))}, {"y", scalar(std::sin(0.9))}});
  auto f = [](const Pack& p) { return 2.0 * (p.at("x").square() + p.at("y").square() - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq([](const Pack& p) {
    return p.at("x").square() + p.at("y").square() - 1.0;
  });
  std::cout << std::left << std::setw(30) << "max_second_order_corrections" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << std::setw(12) << "objective" << "x\n";
  for (std::int64_t k : {0, 1, 2}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_second_order_corrections = k;
    const auto r = optim::optima(f, start, o, cons);
    std::cout << std::left << std::setw(30) << k << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
              << std::fixed << std::setprecision(6) << std::setw(12) << r.evaluation.value.item<double>()
              << as_f64(r.params.at("x")) << "\n";
  }
  return 0;
}
```

Output

```text
max_second_order_corrections  iters  evals  objective   x
0                             8      18     -1.000000   1.000000
1                             7      18     -1.000000   1.000000
2                             7      18     -1.000000   1.000000
```
All three runs reach the minimum, `-1` at `x = 1`. With the correction off the run takes 8 iterations and 18 evaluations; with one or two corrections it takes 7 iterations and the same 18 evaluations. The saving is one iteration, because the problem is small and the starting point is close to the solution, but the cause is exactly the one above, and the correction is cheap insurance on any problem with curved equality constraints.

## Decisions and pitfalls

- **Read the trace as a whole.** A healthy SQP run shows the violation falling by orders of magnitude, the stationarity following, and the full step (`1.00`) accepted, which means the quadratic model predicts the problem well.
- **Do not tune the penalty first.** The solver sets it from the size of the multipliers at each iteration, so a poor starting guess is corrected within a few iterations; an extreme starting weight only costs evaluations.
- **The default line-search settings are rarely the problem.** The default backtracking factor was the cheapest here; if you change a setting, change one at a time and compare the evaluation counts.
- **Leave the second-order correction on for curved constraints.** It is cheap insurance against a rejected good step; the saving is small on an easy problem and large on one with a sharply curved equality.
- **Count evaluations as well as iterations.** Line-search backtracking raises the evaluations without raising the iterations.

Where next: [Subproblems, restoration and stopping](subproblems_restoration_and_stopping.md) covers what happens when the loop struggles and when it should stop.
