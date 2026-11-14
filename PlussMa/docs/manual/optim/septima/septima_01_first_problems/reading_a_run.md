<!-- generated from reading_a_run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reading and watching a run

[Back to Septima 1: First Problems](../septima_01_first_problems.md)

A Septima call returns one result object, and learning to read it is the difference between trusting a run and checking it. The result holds the answer (`params`, one `Series` per block, in the order you gave them), the objective at the answer, two different gradient norms (the gradient of the objective alone and the *stationarity* of the constrained problem, which is the number actually compared with the tolerance), the size of the last step, the worst constraint violation, and the cost (iterations and function evaluations).

The page also shows how a run can end: five exit flags, each produced on purpose, so you know what each looks like and what it promises. And it shows the progress hook, a function called after every iteration with a snapshot of the run, which returns `true` to stop the run and `false` to keep going, exactly as in `optima`; it is the tool for logging, for stopping at a target, and for watching a long run.

## Program 4: Reading the result

The problem is a bounded fit, `0.5 ((a - 2)^2 + (b - 1)^2)` with `a <= 1.5`, so the bound is active at the answer. The program prints each field of the result.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> objective = [](const Pack& p) {
    return 0.5 * ((p.at("a") - 2.0).square() + (p.at("b") - 1.0).square());
  };
  optim::SeptimaConstraints constraints;
  constraints.ubs = {scalar(1.5), scalar(100.0)};  // one bound per block, in block order

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto r = optim::septima(objective, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options, constraints);

  std::cout << std::scientific << std::setprecision(2) << std::boolalpha;
  std::cout << "converged               " << r.converged << "\n";
  std::cout << "exit_flag               " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n";
  std::cout << "message                 " << r.message << "\n";
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "params                  a = " << as_f64(r.params[0]) << ", b = " << as_f64(r.params[1]) << "\n";
  std::cout << "objective_value         " << r.objective_value << "\n";
  std::cout << std::scientific << std::setprecision(2);
  std::cout << "gradient_inf_norm       " << r.gradient_inf_norm << "  (the objective's gradient alone)\n";
  std::cout << "stationarity            " << r.stationarity << "  (with the active bound taken into account)\n";
  std::cout << "step_inf_norm           " << r.step_inf_norm << "  (size of the last step)\n";
  std::cout << "constraint_violation    " << r.constraint_violation << "\n";
  std::cout << "iteration               " << r.iteration << "\n";
  std::cout << "function_evaluations    " << r.function_evaluations << "\n";
  return 0;
}
```

Output

```text
converged               true
exit_flag               1  (1 = Converged)
message                 converged
params                  a = 1.5000, b = 1.0000
objective_value         0.1250
gradient_inf_norm       5.00e-01  (the objective's gradient alone)
stationarity            0.00e+00  (with the active bound taken into account)
step_inf_norm           0.00e+00  (size of the last step)
constraint_violation    0.00e+00
iteration               2
function_evaluations    2
```
In the order you will use them:

- `converged` and `exit_flag` say whether the run met its tests. `message` says why it stopped.
- `params` is the answer, one `Series` per block, in the order you gave them.
- `objective_value` is the objective at the answer.
- `gradient_inf_norm` is the largest component of the gradient of the **objective alone**. Here it is 0.5, not zero: at a constrained optimum the objective still pulls toward the bound.
- `stationarity` is the largest component of the gradient of the constrained problem, with the components that an active bound blocks removed. It is the number the solver compares with `gradient_tolerance`. Here it is 0, which is why the run converged although `gradient_inf_norm` is 0.5. Without constraints it equals `gradient_inf_norm`.
- `step_inf_norm` is the size of the last step; `constraint_violation` is the worst violation of a bound, an equality or an inequality.
- `iteration` and `function_evaluations` are the cost. Without a line search they are equal.
- `state` (not printed) holds the optimizer's memory and the constraint multipliers (chapter 2).

## Program 5: How a run ends

Each way a run can end, produced on purpose.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto quadratic = [](const pluss::Series& x) -> pluss::Series { return (x - 3.0).square().sum(); };
  const auto row = [](const char* what, const optim::SeptimaResult& r) {
    std::cout << std::left << std::setw(40) << what << std::setw(21) << optim::septima_exit_flag_name(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << r.message << "\n";
  };

  {
    optim::SeptimaSolveOptions o;
    o.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    row("L-BFGS on a quadratic", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // default Adam learning rate 0.001, default 200 iterations
    row("Adam with its default settings", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 0.1;
    o.progress_hook = [](const optim::SeptimaProgressSnapshot& s) { return s.iteration >= 7; };  // true stops
    row("a hook that returns true at 7", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // a learning rate so small that nothing changes within the tolerances
    o.step.adam.learning_rate = 1e-18;
    o.max_no_progress_iterations = 5;
    row("a learning rate of 1e-18", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    // log of a negative number is NaN: a large first step leaves the domain.
    auto domain = [](const pluss::Series& x) -> pluss::Series { return x.log().sum() * -1.0 + (x * 0.1).sum(); };
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 100.0;
    o.step.nan_recovery_attempts = 0;
    row("a step into log's undefined region", optim::septima(domain, vec({1.0}), o));
  }

  std::cout << "\nInvalid settings throw:\n";
  try {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = -1.0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  try {
    optim::SeptimaSolveOptions o;
    o.max_iterations = 0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
L-BFGS on a quadratic                   Converged            iterations 3     converged
Adam with its default settings          MaxIterations        iterations 200   max iterations reached
a hook that returns true at 7           Stopped              iterations 7     stopped by progress hook
a learning rate of 1e-18                NoProgress           iterations 6     no progress
a step into log's undefined region      NonFiniteEvaluation  iterations 20    objective became non-finite

Invalid settings throw:
  optim::septima_step_inplace: adam.learning_rate must be positive and finite
  optim::septima: max_iterations must be > 0
```
| Flag | Meaning | In the program |
|---|---|---|
| `Converged` | gradient, step and violation are all within tolerance | L-BFGS on a quadratic, in 3 iterations |
| `MaxIterations` | the budget ran out first | Adam with its defaults, 200 iterations |
| `Stopped` | the progress hook returned `true` | a hook that stops at iteration 7 |
| `NoProgress` | the objective and the step stopped changing before the tolerances were met | a learning rate of 1e-18, after 6 iterations |
| `NonFiniteEvaluation` | the objective or its gradient became NaN or infinite | a step of size 100 into the region where `log` is undefined |

`NoProgress` is declared after `max_no_progress_iterations` (25 by default; the program uses 5) consecutive iterations in which the objective changes by at most `objective_tolerance` and the step is at most `step_tolerance`. Invalid settings do not return a flag: the call throws an error that names the setting, as the last two lines show (a negative learning rate, and a budget of zero iterations).

## Program 6: Watching a run

The progress hook is called after every iteration with a snapshot (iteration, function evaluations, objective, gradient, stationarity, step, violation, message). It returns `true` to stop with `Stopped` and `false` to keep going. This program prints every 150th iteration of an Adam run on the Rosenbrock valley with a learning rate of 0.05 and records when the objective first fell below 0.01.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  };
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.05;
  options.max_iterations = 1500;
  std::int64_t first_below = 0;
  options.progress_hook = [&](const optim::SeptimaProgressSnapshot& s) {
    if (s.iteration % 150 == 0 || s.iteration == 1)
      std::cout << std::setw(6) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient "
                << s.gradient_inf_norm << "  step " << s.step_inf_norm << "\n";
    if (first_below == 0 && s.objective < 0.01) first_below = s.iteration;
    return false;
  };
  const auto r = optim::septima(rosenbrock, optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}}), options);
  std::cout << "the objective first fell below 0.01 at iteration " << first_below << "; the run used " << r.iteration << " iterations\n";
  return 0;
}
```

Output

```text
     1  objective 2.420e+01  gradient 2.156e+02  step 5.000e-02
   150  objective 3.181e+00  gradient 1.748e+00  step 4.609e-03
   300  objective 1.231e+00  gradient 2.193e+00  step 5.793e-03
   450  objective 2.439e-01  gradient 6.607e-01  step 2.388e-03
   600  objective 6.592e-02  gradient 2.527e-01  step 1.574e-03
   750  objective 1.941e-02  gradient 1.174e-01  step 9.795e-04
   900  objective 5.552e-03  gradient 5.773e-02  step 5.908e-04
  1050  objective 1.464e-03  gradient 2.829e-02  step 3.398e-04
  1200  objective 3.438e-04  gradient 1.337e-02  step 1.837e-04
  1350  objective 7.014e-05  gradient 5.962e-03  step 9.220e-05
  1500  objective 1.216e-05  gradient 2.468e-03  step 4.255e-05
the objective first fell below 0.01 at iteration 831; the run used 1500 iterations
```
The objective falls from 24.2 to 1.2e-05 over 1500 iterations, with the gradient falling from 216 to 2.5e-03 and the step shrinking as it goes. It first fell below 0.01 at iteration 831, which is the kind of thing a hook can act on: stop at a target value, log to a file, or adjust something outside the solver. A trace like this is also the quickest way to see whether a learning rate is too small (a slow, steady fall, as here), too large (the objective oscillating), or about right.

### A ready-made report

Writing a hook by hand (the previous program) is the way to react to a run, for example to stop it early. To simply watch one, `reports::iter(std::cout, precision, every)` is a hook that prints a line per iteration, or per `every`-th iteration; it is the same `reports::iter` that `optima` uses. It is a small object that can be called with either solver's progress snapshot, so the solver you assign it to decides which one it receives and therefore which columns are printed; no setting is needed. This run uses L-BFGS on the Rosenbrock valley and prints every fifth iteration.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  const auto start = pluss::make_series(std::vector<double>{-1.2, 1.0}, {2}).to(f64);

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  options.progress_hook = optim::reports::iter(std::cout, 3, 5);

  const auto rosenbrock = [](const pluss::Series& x) {
    const auto a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };
  const auto result = optim::septima(rosenbrock, start, options);
  std::cout << "finished after " << result.iteration << " iterations: " << optim::septima_exit_flag_name(result.exit_flag) << "\n";
  return 0;
}
```

Output

```text
iter 1  objective 2.420e+01  gradient 2.156e+02  step 2.156e+02
iter 5  objective 4.494e+00  gradient 2.312e+01  step 1.507e-02
iter 10  objective 4.162e+00  gradient 6.479e+00  step 1.418e-01
iter 15  objective 2.633e+00  gradient 7.827e+00  step 2.898e-02
iter 20  objective 1.820e+00  gradient 1.252e+01  step 8.943e-02
iter 25  objective 1.265e+00  gradient 2.004e+00  step 1.531e+00
iter 30  objective 1.244e+00  gradient 2.041e+00  step 4.660e-03
iter 35  objective 1.226e+00  gradient 2.367e+00  step 1.733e-03
iter 40  objective 1.032e+00  gradient 6.677e+00  step 9.468e-02
iter 45  objective 4.328e-01  gradient 1.282e+00  step 7.204e-01
iter 50  objective 3.736e-01  gradient 2.244e+00  step 3.046e-02
iter 55  objective 2.993e-01  gradient 1.674e+00  step 3.243e-03
iter 60  objective 2.653e-01  gradient 3.139e+00  step 3.728e-02
iter 65  objective 3.881e-02  gradient 2.623e-01  step 1.578e-01
iter 70  objective 5.086e-03  gradient 7.959e-02  step 3.376e-02
iter 75  objective 5.145e-06  gradient 6.019e-02  step 2.913e-03
iter 80  objective 1.645e-20  gradient 3.161e-09  step 2.036e-10
finished after 81 iterations: Converged
```
The objective falls from 24.2 at the first iteration to 4.5 by iteration 5 and 1.0 by iteration 40, then drops quickly as the iterates reach the floor of the valley: 0.0051 at iteration 70, 5.1e-6 at iteration 75 and 1.6e-20 at iteration 80, where the gradient is 3.2e-9 and the step 2e-10. Those are within every tolerance, so the run ends at iteration 81 with `Converged`. This is what the report is for: the gradient and step columns show the approach to the answer, and the iteration at which they collapse tells you when the valley was found.

## Decisions and pitfalls

- **Read `stationarity`, not `gradient_inf_norm`, at a constrained optimum.** The objective's own gradient is not zero where a bound is active; the stationarity removes the components an active bound blocks.
- **Check the flag.** `Converged` means the gradient, the step and the violation are all within tolerance; `MaxIterations`, `Stopped`, `NoProgress` and `NonFiniteEvaluation` each have a different remedy.
- **Invalid settings throw, they do not return a flag.** A setting that cannot be right (a negative tolerance) throws an error that names the setting.
- **A hook can be used to act, not only to look.** It can stop at a target value, log to a file or adjust something outside the solver; keep it cheap, since it runs every iteration.

Where next: [Chapter 2](../septima_02_constraints.md) adds bounds, equalities and inequalities.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="first_problems.html">
    <span class="label">Previous</span>
    <span class="title">First problems with Septima</span>
  </a>
</div>
<!-- apiref:nav:end -->
