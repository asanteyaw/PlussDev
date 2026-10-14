<!-- generated from optima_02_reading_results.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 2: Reading Results and Watching a Run

A solver run can end in many ways, and only one of them means "here is your answer". This chapter is about telling them apart: which exit flag was returned and what each one promises, how the stall category narrows down a failure, what every field of the structured report means, how to watch a run as it happens, how to stop it early, and how to read the Lagrange multipliers. After it you can write code that trusts a result only when it should, and that explains a failure when it does not.

Every program is complete and its output is the output of that program.

## The mental model

Four independent pieces of information come back from every call. Keep them separate.

- **The exit flag** (`result.exit_flag`) is the verdict, one of nine values. It is the thing to branch on.
- **`result.converged`** is a convenience: it is true exactly when the exit flag is `Converged`. It is not true merely because the solver stopped.
- **The stall category** (`result.output.stall_category`) names the kind of difficulty behind a stop that was not a success. It narrows a failure down; it never changes the verdict.
- **The message** (`result.message`) is human-readable text for logs. Its wording can change between versions. Never branch on it.

Whatever the verdict, `result.params` always holds a point: the last iterate, or, for some stops, the best feasible point the run found. It is only a solution when the flag says so.

## The nine exit flags

| Flag | Value | What it promises | What `result.params` holds |
|---|---|---|---|
| `Converged` | 1 | Constraint violation and first-order optimality are within tolerance, or the iterate is a stationary feasible point. | The solution. |
| `MaxIterations` | 0 | The iteration budget was spent first. | The last iterate; `best_feasible` may be better. |
| `Stopped` | -1 | Your progress hook returned `true`. | The point at which you stopped. |
| `NoFeasiblePoint` | -2 | The interior method could not find a strictly feasible region to start from. | The start, nudged inward. |
| `ObjectiveTargetReached` | -3 | A feasible point at or below `objective_limit` was found. | That point. |
| `NonFiniteEvaluation` | -4 | The objective, gradient, a constraint or a Jacobian was not a finite number. | The point at which the non-finite value appeared. |
| `InvalidProblem` | -5 | A callback broke its contract (wrong result shape, a thrown error). | The start. |
| `NoProgress` | -6 | The line search, the recovery step or the step to the boundary could not make progress. | The last iterate. |
| `ConstraintStationary` | -7 | The constraint violation stopped decreasing at a point that is still infeasible. | The last iterate, infeasible. |

Only the first row is a success. `ObjectiveTargetReached` is a success you asked for, but it does not claim optimality, which is why `converged` is false for it.

## The four algorithms side by side

Choosing an algorithm is the subject of the next chapter. For now, the first program shows that all four share one result type, so every report in this chapter works the same for each. It also shows how little they differ in what they return on an easy problem, and how much in the work they do to get there.

```cpp
// The same constrained problem solved by each of the four algorithms. They share the result type,
// so the answer, the work done and the stopping message can be compared directly.
// Problem: minimize (x0 - 2)^2 + (x1 - 1)^2 subject to x0 <= 1.5 and x1 <= 0.8.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto target = optim::trace(vec({2.0, 1.0}), false);
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - target)); };
  const auto constraints = optim::constraints().ubs(vec({1.5, 0.8}));

  const std::vector<std::pair<std::string, optim::OptimaAlgorithm>> algorithms = {
      {"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
      {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::fixed << std::setprecision(4);
  for (const auto& [name, algorithm] : algorithms) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(objective, vec({0.5, 0.5}), options, constraints);
    const auto x = r.params.at("x").to_vec<double>();
    std::cout << std::left << std::setw(17) << name << std::setw(24) << ("label=" + r.output.algorithm) << std::right
              << "x=[" << x[0] << ", " << x[1] << "]  flag=" << std::setw(2) << static_cast<int>(r.exit_flag)
              << "  iterations=" << std::setw(3) << r.iteration << "  evaluations=" << std::setw(3)
              << r.function_evaluations << "\n";
  }
  return 0;
}
```

### Output

```text
MeritMethod      label=merit-method      x=[1.5000, 0.8000]  flag= 1  iterations=  2  evaluations=  3
SequentialQP     label=sqp               x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
WorkingSet       label=working-set       x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
InternalProtocol label=internal-protocol x=[1.5000, 0.8000]  flag= 1  iterations=  5  evaluations=  8
```
All four agree on the answer, the corner `(1.5, 0.8)`, and all four exit with `Converged`. They differ in effort: the merit method needs two iterations and three evaluations because it handles bounds by projection, while sequential QP and the working-set method need a single iteration, because their first quadratic subproblem already identifies the corner, and the interior method takes five iterations and eight evaluations, since a barrier method approaches a bound from inside. `output.algorithm` carries each method's label.

## A guided tour

### Program 2: the flags that mean a healthy run

Four flags describe a run that behaved as designed: it finished, it ran out of its budget, you stopped it, or it reached a target you set. This program triggers each one and prints the flag, the `converged` convenience, and the iteration count.

```cpp
// The exit flag says why the solver stopped. This program triggers the four flags that mean the run
// was healthy: it finished (Converged), it ran out of iterations (MaxIterations), the caller's hook
// asked it to stop (Stopped), or it reached an objective value the caller said was good enough
// (ObjectiveTargetReached).
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;

std::string flag_name(optim::OptimaExitFlag flag) {
  switch (flag) {
    case optim::OptimaExitFlag::Converged: return "Converged";
    case optim::OptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::OptimaExitFlag::Stopped: return "Stopped";
    case optim::OptimaExitFlag::NoFeasiblePoint: return "NoFeasiblePoint";
    case optim::OptimaExitFlag::ObjectiveTargetReached: return "ObjectiveTargetReached";
    case optim::OptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::OptimaExitFlag::InvalidProblem: return "InvalidProblem";
    case optim::OptimaExitFlag::NoProgress: return "NoProgress";
    case optim::OptimaExitFlag::ConstraintStationary: return "ConstraintStationary";
  }
  return "?";
}

void report(const std::string& label, const optim::OptimaState& r) {
  std::cout << label << ": " << flag_name(r.exit_flag) << " (" << static_cast<int>(r.exit_flag)
            << "), converged=" << std::boolalpha << r.converged << ", iterations=" << r.iteration << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  // Converged: the usual outcome.
  report("finished normally", optim::optima(objective, vec({0.0, 0.0})));

  // MaxIterations: the iteration budget is spent before the tolerances are met.
  optim::OptimaOptions few;
  few.sequential_qp.max_iterations = 1;
  report("one iteration allowed", optim::optima(objective, vec({0.0, 0.0}), few, disc));

  // Stopped: the progress hook returned true, asking for an orderly stop.
  optim::OptimaOptions stop_early;
  int calls = 0;
  stop_early.progress_hook = [&](const optim::OptimaProgressSnapshot&) { return ++calls >= 2; };
  report("hook asked to stop", optim::optima(objective, vec({0.0, 0.0}), stop_early));

  // ObjectiveTargetReached: a feasible point at or below objective_limit ends the run at once.
  optim::OptimaOptions good_enough;
  good_enough.sequential_qp.objective_limit = 100.0;
  report("objective limit 100", optim::optima(objective, vec({0.0, 0.0}), good_enough));
  return 0;
}
```

### Output

```text
finished normally: Converged (1), converged=true, iterations=1
one iteration allowed: MaxIterations (0), converged=false, iterations=1
hook asked to stop: Stopped (-1), converged=false, iterations=1
objective limit 100: ObjectiveTargetReached (-3), converged=false, iterations=0
```
Only the first run is `converged`. The second spent its single allowed iteration, so the flag is `MaxIterations` and the iterate is not a solution. The third stopped because the hook returned `true` on its second call; `Stopped` always means the caller asked. The fourth ended at iteration zero because the start already satisfied `objective_limit = 100`. A feasible point at or below the limit ends the run at once with `ObjectiveTargetReached`.

### Program 3: the five failure flags

Five flags mean the solver could not deliver. Each is triggered here by a specific condition, and the result still carries the last point reached so you can inspect it.

```cpp
// The five flags that mean the solver could not deliver a solution. Each comes with the program
// state that triggers it, and the result still carries the last point reached.
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

std::string flag_name(optim::OptimaExitFlag flag) {
  switch (flag) {
    case optim::OptimaExitFlag::Converged: return "Converged";
    case optim::OptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::OptimaExitFlag::Stopped: return "Stopped";
    case optim::OptimaExitFlag::NoFeasiblePoint: return "NoFeasiblePoint";
    case optim::OptimaExitFlag::ObjectiveTargetReached: return "ObjectiveTargetReached";
    case optim::OptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::OptimaExitFlag::InvalidProblem: return "InvalidProblem";
    case optim::OptimaExitFlag::NoProgress: return "NoProgress";
    case optim::OptimaExitFlag::ConstraintStationary: return "ConstraintStationary";
  }
  return "?";
}

void report(const std::string& label, const optim::OptimaState& r) {
  std::cout << label << ": " << flag_name(r.exit_flag) << " (" << static_cast<int>(r.exit_flag)
            << "), max violation " << r.evaluation.max_constraint_violation << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  // No point satisfies x0^2 + x1^2 + 1 <= 0, and none satisfies x0^2 + x1^2 + 1 = 0.
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  // NoFeasiblePoint: the interior-point method needs a strictly feasible start region and finds none.
  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  report("interior method, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), interior, impossible_inequality));

  // NoProgress: the recovery step could not reduce the violation, so the run gave up.
  report("sequential QP, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  // ConstraintStationary: the violation stopped decreasing at a point that is still infeasible.
  report("interior method, impossible equality", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  // NonFiniteEvaluation: the objective is not a number at the start (log of a negative value).
  report("log of a negative start",
         optim::optima([](const optim::Trace& x) { return optim::sum(optim::log(x)); }, vec({-1.0, 1.0})));

  // InvalidProblem: the objective returned two values instead of one.
  report("objective returns a vector", optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0})));
  return 0;
}
```

### Output

```text
interior method, impossible inequality: NoFeasiblePoint (-2), max violation 1
sequential QP, impossible inequality: NoProgress (-6), max violation 1
interior method, impossible equality: ConstraintStationary (-7), max violation 1
log of a negative start: NonFiniteEvaluation (-4), max violation 0
objective returns a vector: InvalidProblem (-5), max violation 0
```
The impossible inequality `x0^2 + x1^2 + 1 <= 0` produces two different verdicts depending on the method. The interior method, which needs a strictly feasible region to start from, returns `NoFeasiblePoint`. Sequential QP tries to recover and fails, returning `NoProgress`. The impossible equality returns `ConstraintStationary` from the interior method: the equality residual can no longer be reduced (the gradient of its squared norm vanishes), so no step can restore feasibility, and the violation stays at `1`. A `log` of a negative start gives `NonFiniteEvaluation`, and an objective that returns a vector gives `InvalidProblem` with no exception thrown.

### Program 4: the ten stall categories

The exit flag is coarse. The stall category says what went wrong underneath. There are ten, and this program triggers each. The scenario text describes the cause, and the line below it shows the category name and the flag it came with.

```cpp
// Beyond the exit flag, every result carries a stall category that says what kind of difficulty ended
// the run. This program triggers each of the ten categories and prints its name, the exit flag, and the
// measure the solver recorded. A healthy run reports "none".
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

void report(const std::string& scenario, const optim::OptimaState& r) {
  std::cout << scenario << "\n    stall=" << optim::optima_stall_category_name(r.output.stall_category)
            << "  flag=" << static_cast<int>(r.exit_flag) << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;

  report("a normal solve", optim::optima(objective, vec({0.0, 0.0})));

  optim::OptimaOptions hook;
  hook.progress_hook = [](const optim::OptimaProgressSnapshot&) { return true; };
  report("the progress hook asks to stop", optim::optima(objective, vec({0.0, 0.0}), hook));

  report("the objective returns a vector",
         optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0})));

  report("the objective is not finite at the start",
         optim::optima([](const optim::Trace& x) { return optim::sum(optim::log(x)); }, vec({-1.0, 1.0})));

  optim::OptimaOptions merit;
  merit.algorithm = optim::OptimaAlgorithm::MeritMethod;
  report("merit method on an impossible inequality", optim::optima(objective, vec({0.0, 0.0}), merit, impossible_inequality));

  optim::OptimaOptions no_step = interior;
  no_step.internal_protocol.fraction_to_boundary = 0.0;
  report("no step is allowed toward the boundary", optim::optima(objective, vec({0.0, 0.0}), no_step, disc));

  report("sequential QP cannot restore feasibility", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  report("interior method stalls on an infeasible equality",
         optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  optim::OptimaOptions phase = interior;
  phase.internal_protocol.enable_feasibility_phase = true;
  report("feasibility phase cannot reduce the violation", optim::optima(objective, vec({1.0, 1.0}), phase, impossible_equality));

  optim::OptimaOptions short_run = interior;
  short_run.internal_protocol.max_iterations = 2;
  report("interior method stops with barrier complementarity unresolved",
         optim::optima(objective, vec({0.1, 0.1}), short_run, disc));

  optim::OptimaOptions one_iteration;
  one_iteration.sequential_qp.max_iterations = 1;
  report("the iteration budget is spent", optim::optima(objective, vec({0.0, 0.0}), one_iteration, disc));
  return 0;
}
```

### Output

```text
a normal solve
    stall=none  flag=1
the progress hook asks to stop
    stall=user-stop  flag=-1
the objective returns a vector
    stall=invalid-callback-contract  flag=-5
the objective is not finite at the start
    stall=nonfinite-callback-region  flag=-4
merit method on an impossible inequality
    stall=search-direction-collapse  flag=-6
no step is allowed toward the boundary
    stall=boundary-limited  flag=-6
sequential QP cannot restore feasibility
    stall=recovery-failure  flag=-6
interior method stalls on an infeasible equality
    stall=infeasible-stationary  flag=-7
feasibility phase cannot reduce the violation
    stall=feasibility-failure  flag=-6
interior method stops with barrier complementarity unresolved
    stall=complementarity-stagnation  flag=0
the iteration budget is spent
    stall=iteration-limit  flag=0
```
| Category | What it means |
|---|---|
| `none` | The run ended healthily. |
| `user-stop` | The progress hook asked to stop. |
| `invalid-callback-contract` | A callback returned the wrong shape or threw. |
| `nonfinite-callback-region` | A callback returned `NaN` or infinity. |
| `search-direction-collapse` | The line search could not find an acceptable step. |
| `boundary-limited` | No step was possible toward a bound or the linearized constraints. |
| `infeasible-stationary` | The constraint violation is at a stationary point that is still infeasible. |
| `complementarity-stagnation` | The interior method hit its budget with the barrier complementarity still large against stationarity. |
| `feasibility-failure` | The feasibility phase could not reduce the violation. |
| `recovery-failure` | The restoration or recovery step failed. |
| `iteration-limit` | The iteration budget was spent. |

`boundary-limited` needs an unusual setting to appear: here the interior method's `fraction_to_boundary = 0` forbids any step toward a constraint boundary. It is the category to look for when a run on a boxed problem refuses to move.

### Program 5: every field of the report

`result.output` is the structured report. This program prints each field of one solve so you can match it against the table below.

```cpp
// OptimaOutput is the structured report inside every result. This program prints every field of
// one solve so each can be matched to the table in the text. The problem is a disc-constrained fit
// solved by sequential QP.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, disc);
  const auto& o = r.output;

  std::cout << std::boolalpha << std::setprecision(4) << std::scientific;
  std::cout << "algorithm                             " << o.algorithm << "\n";
  std::cout << "iterations                            " << o.iterations << "\n";
  std::cout << "function_evaluations                  " << o.function_evaluations << "\n";
  std::cout << "constraint_violation                  " << o.constraint_violation << "\n";
  std::cout << "first_order_optimality                " << o.first_order_optimality << "\n";
  std::cout << "step_size                             " << o.step_size << "\n";
  std::cout << "line_search_fraction                  " << o.line_search_fraction << "\n";
  std::cout << "backend_parameter                     " << o.backend_parameter << "\n";
  std::cout << "complementarity                       " << o.complementarity << "\n";
  std::cout << "krylov_iterations                     " << o.krylov_iterations << "\n";
  std::cout << "best_feasible_available               " << o.best_feasible_available << "\n";
  std::cout << "returned_best_feasible                " << o.returned_best_feasible << "\n";
  std::cout << "best_feasible_value                   " << o.best_feasible_value << "\n";
  std::cout << "best_feasible_constraint_violation    " << o.best_feasible_constraint_violation << "\n";
  std::cout << "best_feasible_first_order_optimality  " << o.best_feasible_first_order_optimality << "\n";
  std::cout << "multiplier_max_norm                   " << o.multiplier_max_norm << "\n";
  std::cout << "active_multiplier_count               " << o.active_multiplier_count << "\n";
  std::cout << "detail_message                        " << o.detail_message << "\n";
  std::cout << "stall_category                        " << optim::optima_stall_category_name(o.stall_category) << "\n";
  std::cout << "stall_measure                         " << o.stall_measure << "\n";
  std::cout << "stall_reference                       " << o.stall_reference << "\n";
  return 0;
}
```

### Output

```text
algorithm                             sqp
iterations                            4
function_evaluations                  9
constraint_violation                  0.0000e+00
first_order_optimality                1.1465e-08
step_size                             1.0000e+00
line_search_fraction                  1.0000e+00
backend_parameter                     1.0000e+01
complementarity                       0.0000e+00
krylov_iterations                     0
best_feasible_available               true
returned_best_feasible                true
best_feasible_value                   1.0515e+01
best_feasible_constraint_violation    0.0000e+00
best_feasible_first_order_optimality  4.5858e+00
multiplier_max_norm                   3.2426e+00
active_multiplier_count               1
detail_message                        optimality and feasibility tolerances satisfied
stall_category                        none
stall_measure                         0.0000e+00
stall_reference                       0.0000e+00
```
| Field | Meaning |
|---|---|
| `algorithm` | The method's label: `merit-method`, `sqp`, `working-set` or `internal-protocol`. |
| `iterations`, `function_evaluations` | Major iterations and the number of objective evaluations. The second is far larger when line searches or recovery steps run. |
| `constraint_violation` | The largest violation of any constraint at the returned point. |
| `first_order_optimality` | How far the returned point is from satisfying the optimality conditions. For sequential QP and the working-set method it is the largest component of the Lagrangian gradient at the returned point, using the multiplier estimate in `result.multipliers`. |
| `step_size`, `line_search_fraction` | The fraction of the full step the last iteration accepted. |
| `backend_parameter` | The method's own control parameter: the penalty weight for the QP methods and the merit method, the barrier parameter for the interior method. It stayed at `10`, its starting value, because the penalty is raised only when constraints stay violated. |
| `complementarity` | The complementarity residual; meaningful for the interior method, zero otherwise. |
| `krylov_iterations` | Iterations spent in the iterative linear solver, when it is used. |
| `best_feasible_available`, `returned_best_feasible` | Whether a feasible checkpoint was recorded, and whether the answer in `params` is that checkpoint. |
| `best_feasible_value`, `best_feasible_constraint_violation`, `best_feasible_first_order_optimality` | The checkpoint's objective value, violation and stationarity. Infinity when no checkpoint exists. |
| `multiplier_max_norm`, `active_multiplier_count` | The largest multiplier magnitude, and how many constraints are active. |
| `detail_message` | The expanded message, including the stall diagnostic for a failed run. |
| `stall_category`, `stall_measure`, `stall_reference` | The stall category, the quantity that triggered it, and the threshold it was compared with. |

The solve is a success: `first_order_optimality` is `1e-8`, so the returned point satisfies the optimality conditions to high accuracy in only nine evaluations. `multiplier_max_norm` is `3.2426`, the multiplier of the one active constraint.

## Watching a run

### Program 6: printing progress

`reports::trace` turns any output stream into a progress hook that prints one line per iteration. `reports::iter` is the short form. `ProgressTracePreset` switches each part of a line on or off.

```cpp
// reports::trace turns any stream into a progress hook that prints one line per iteration, so a run
// can be watched as it happens. reports::iter is the short form: iterations only, no start or
// final line, no message. ProgressTracePreset switches individual parts on and off.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  std::cout << "-- reports::trace, default preset --\n";
  optim::OptimaOptions full;
  full.progress_hook = optim::reports::trace(std::cout);
  (void)optim::optima(objective, vec({0.0, 0.0}), full, disc);

  std::cout << "-- reports::iter, three decimals --\n";
  optim::OptimaOptions brief;
  brief.progress_hook = optim::reports::iter(std::cout, 3);
  (void)optim::optima(objective, vec({0.0, 0.0}), brief, disc);

  std::cout << "-- a custom preset: no start line, no final line, no message, no Krylov count --\n";
  optim::reports::ProgressTracePreset preset;
  preset.include_start = false;
  preset.include_done = false;
  preset.include_message = false;
  preset.include_krylov = false;
  preset.precision = 4;
  optim::OptimaOptions custom;
  custom.progress_hook = optim::reports::trace(std::cout, preset);
  (void)optim::optima(objective, vec({0.0, 0.0}), custom, disc);
  return 0;
}
```

### Output

```text
-- reports::trace, default preset --
iter=0 evals=1 objective=18.000000 viol=0.000000 stationarity=6.000000 backend=10.000000 step=0.000000 krylov=0 stall=none msg="initialized"
iter=1 evals=6 objective=10.125000 viol=0.125000 stationarity=1.000000 backend=10.000000 step=0.125000 krylov=0 stall=none msg="step accepted"
iter=2 evals=7 objective=10.503472 viol=0.003472 stationarity=0.018519 backend=10.000000 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=3 evals=8 objective=10.514709 viol=0.000003 stationarity=0.002169 backend=10.000000 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000002 backend=10.000000 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000000 backend=10.000000 step=1.000000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000000 backend=10.000000 step=1.000000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
-- reports::iter, three decimals --
iter=1 evals=6 objective=10.125 viol=0.125 stationarity=1.000 backend=10.000 step=0.125 stall=none
iter=2 evals=7 objective=10.503 viol=0.003 stationarity=0.019 backend=10.000 step=1.000 stall=none
iter=3 evals=8 objective=10.515 viol=0.000 stationarity=0.002 backend=10.000 step=1.000 stall=none
iter=4 evals=9 objective=10.515 viol=0.000 stationarity=0.000 backend=10.000 step=1.000 stall=none
iter=4 evals=9 objective=10.515 viol=0.000 stationarity=0.000 backend=10.000 step=1.000 stall=none
-- a custom preset: no start line, no final line, no message, no Krylov count --
iter=1 evals=6 objective=10.1250 viol=0.1250 stationarity=1.0000 backend=10.0000 step=0.1250 stall=none
iter=2 evals=7 objective=10.5035 viol=0.0035 stationarity=0.0185 backend=10.0000 step=1.0000 stall=none
iter=3 evals=8 objective=10.5147 viol=0.0000 stationarity=0.0022 backend=10.0000 step=1.0000 stall=none
iter=4 evals=9 objective=10.5147 viol=0.0000 stationarity=0.0000 backend=10.0000 step=1.0000 stall=none
iter=4 evals=9 objective=10.5147 viol=0.0000 stationarity=0.0000 backend=10.0000 step=1.0000 stall=none
```
The default trace prints a start line, one line per iteration, and a final line, each with the objective, violation, stationarity, backend parameter, step and message. `reports::iter` drops the start and final lines, the Krylov count and the message, and takes the number of decimals. The custom preset does the same with four decimals. Notice the two lines that repeat at the end of the first trace: the last iteration is reported once as an iteration and once as `done`.

Read a trace by watching two columns. A run that is working shows `viol` falling to zero and `stationarity` falling at the end. Here both fall by orders of magnitude on every iteration, `viol` from `0.125` to `0.0035` to `3e-06` to zero and `stationarity` from `1` to `0.019` to `0.002` to `2e-06`: the signature of a quadratic-model method closing in on a solution. A trace that instead jumps back up, or stalls, is the signal to look at the stall category.

### Program 7: writing your own hook

A hook is any function from a snapshot to a bool. The snapshot carries the stage, the current point, the full evaluation, and the work counters. This program records the objective at every iteration and stops the run as soon as the point is feasible and good enough.

```cpp
// A progress hook is any function from a snapshot to a bool. It sees the stage (start, each
// iteration, done), the current point, the evaluation there, and the work counters. Returning true
// asks the solver to stop at the next opportunity. This hook records the objective history and stops
// the run as soon as the point is feasible and the objective is below 10.55.
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  std::vector<double> history;
  optim::OptimaOptions options;
  options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
    const double value = s.evaluation.value.item<double>();
    std::cout << std::string(optim::optima_progress_stage_name(s.stage)) << "  iteration " << s.iteration
              << "  objective " << value << "  violation " << s.evaluation.max_constraint_violation << "\n";
    if (s.stage == optim::OptimaProgressStage::Iteration) history.push_back(value);
    return s.stage == optim::OptimaProgressStage::Iteration && s.evaluation.max_constraint_violation <= 1e-6 && value < 10.55;
  };

  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);
  std::cout << "recorded " << history.size() << " iteration values\n";
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", stopped at iteration " << r.iteration << "\n";
  const auto x = r.params.at("x").to_vec<double>();
  std::cout << "returned point [" << x[0] << ", " << x[1] << "]\n";
  return 0;
}
```

### Output

```text
start  iteration 0  objective 18  violation 0
iter  iteration 1  objective 10.125  violation 0.125
iter  iteration 2  objective 10.5035  violation 0.00347222
iter  iteration 3  objective 10.5147  violation 3.00243e-06
iter  iteration 4  objective 10.5147  violation 0
done  iteration 4  objective 10.5147  violation 0
recorded 4 iteration values
exit flag -1, stopped at iteration 4
returned point [0.707107, 0.707107]
```
The hook sees `start` once, `iter` for each iteration, and `done` once. It returned `true` at iteration 4, the first time the iterate was feasible and below `10.55`. The run ended with `Stopped`, the point it returned is the feasible one, and the `done` snapshot fires even when the hook ends the run. A hook can read `s.params` to log the point, or `s.evaluation.gradient` for the gradient, so it doubles as a monitor.

### Program 8: which hook runs

You can set one hook on `OptimaOptions::progress_hook` and have it apply to whichever algorithm you choose, or set a hook on one algorithm's own options. When both are set the specific one wins.

```cpp
// A hook can be set once for any algorithm on OptimaOptions::progress_hook, or on one algorithm's own
// option block. The algorithm-specific hook wins when both are set; the general one fills in when the
// algorithm's own is empty. Counting calls shows which hook ran.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };

  int general = 0, specific = 0;
  optim::OptimaOptions options;
  options.progress_hook = [&](const optim::OptimaProgressSnapshot&) { ++general; return false; };
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "general hook only:      general=" << general << " specific=" << specific << "\n";

  general = 0;
  options.sequential_qp.progress_hook = [&](const optim::OptimaProgressSnapshot&) { ++specific; return false; };
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "both hooks set:         general=" << general << " specific=" << specific << "\n";

  general = specific = 0;
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "other algorithm chosen: general=" << general << " specific=" << specific << "\n";
  return 0;
}
```

### Output

```text
general hook only:      general=4 specific=0
both hooks set:         general=0 specific=4
other algorithm chosen: general=4 specific=0
```
With only the general hook set it received all four snapshots. After adding a sequential QP hook, the specific hook received them and the general one received none. Choosing a different algorithm falls back to the general hook, because the sequential QP hook belongs to a method that is not running.

## Best feasible points and multipliers

### Program 9: the best feasible checkpoint

A run that ends without converging may have visited a better point than its last. The solver keeps the best feasible point it saw. `result.best_feasible` holds it and `output.best_feasible_*` summarizes it. `returned_best_feasible` tells you whether the answer in `params` is the checkpoint or the final iterate.

```cpp
// When a run ends without converging, the last iterate is not always the best point it saw. The solver
// keeps the best feasible point it found; result.best_feasible holds it, and output.best_feasible_*
// report its value, violation and stationarity. returned_best_feasible tells you whether the answer in
// result.params is that checkpoint. Here sequential QP is stopped after three iterations, while its
// iterate is still a hair infeasible.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.max_iterations = 3;
  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << std::boolalpha;
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", converged " << r.converged << "\n";
  std::cout << "returned answer: objective " << r.evaluation.value.item<double>() << ", violation "
            << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible available: " << r.output.best_feasible_available << "\n";
  std::cout << "answer is the checkpoint: " << r.output.returned_best_feasible << "\n";
  if (r.best_feasible) {
    const auto x = r.best_feasible->params.at("x").to_vec<double>();
    std::cout << "checkpoint point [" << x[0] << ", " << x[1] << "]\n";
    std::cout << "checkpoint objective " << r.best_feasible->value.item<double>() << ", violation "
              << r.best_feasible->max_constraint_violation << "\n";
    std::cout << "report matches checkpoint: "
              << (r.output.best_feasible_value == r.best_feasible->value.item<double>()) << "\n";
  }
  return 0;
}
```

### Output

```text
exit flag 0, converged false
returned answer: objective 10.5147, violation 3.00243e-06
best feasible available: true
answer is the checkpoint: false
checkpoint point [0, 0]
checkpoint objective 18, violation 0
report matches checkpoint: true
```
Sequential QP was stopped after three iterations, so the flag is `MaxIterations` and `converged` is false. The returned answer is excellent in value (`10.5147`) but infeasible by `3e-06`, just over the default tolerance, because this method approaches feasibility from outside. The only feasible point it ever visited is the starting point, `(0, 0)` with objective `18`, so that is the checkpoint, and the answer in `params` is not that checkpoint (`false`). Here the checkpoint is feasible but far worse, which shows the trade-off: neither is automatically the one to use. A program that wants a usable point from a run that did not converge should read `best_feasible`, and check its violation, rather than `params`.

### Program 10: reading the multipliers

A Lagrange multiplier measures how hard a constraint pushes on the answer. For an inequality it is the first-order price: relaxing the limit by one unit lowers the objective by about that amount. The result carries six multiplier groups, one per constraint family. This program uses three problems whose multipliers can be worked out by hand and checks each.

```cpp
// Lagrange multipliers measure how hard each constraint pushes on the answer. A positive inequality
// multiplier is the first-order price of the constraint: relaxing its limit by one unit lowers the
// objective by about that much. Three problems with known multipliers, one per constraint family. A family the problem does not use has an empty multiplier Series; an inactive constraint has multiplier zero.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  std::cout << std::fixed << std::setprecision(4);

  // Bounds x <= (2, 2.5): the gradient at the answer is (-2, -1), so the upper multipliers are (2, 1).
  const auto bounded = optim::optima(objective, vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));
  const auto upper = bounded.multipliers.upper_bounds.to_vec<double>();
  const auto lower = bounded.multipliers.lower_bounds.to_vec<double>();
  std::cout << "bounds:            upper = [" << upper[0] << ", " << upper[1] << "] (expected 2, 1); lower multipliers: "
            << lower.size() << " (no lower bound set)\n";

  // x0 + x1 <= 4: at (2, 2) the gradient is (-2, -2) and the constraint row is (1, 1), so the multiplier is 2.
  const auto linear = optim::optima(
      objective, vec({0.0, 0.0}), {},
      optim::constraints().ln_ineq(pluss::make_series(std::vector<double>{1, 1}, {1, 2}).to(f64), vec({4.0})));
  std::cout << "linear inequality: " << linear.multipliers.linear_inequalities.to_vec<double>()[0]
            << " (expected 2)\n";

  // x0^2 + x1^2 = 1: at (1,1)/sqrt(2) the gradient is -(3 - 0.7071) * (2, 2); the row is 2*(0.7071, 0.7071).
  const auto nonlinear = optim::optima(
      objective, vec({1.0, 0.0}), {},
      optim::constraints().nl_eq([](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; }));
  std::cout << "nonlinear equality: " << nonlinear.multipliers.nonlinear_equalities.to_vec<double>()[0]
            << " (expected 3.2426)\n";
  return 0;
}
```

### Output

```text
bounds:            upper = [2.0000, 1.0000] (expected 2, 1); lower multipliers: 0 (no lower bound set)
linear inequality: 2.0000 (expected 2)
nonlinear equality: 3.2426 (expected 3.2426)
```
The bound problem has gradient `(-2, -1)` at its answer, so the upper multipliers are `(2, 1)` and the lower group is empty because no lower bound was set. The linear inequality has multiplier `2`, the price of tightening `x0 + x1 <= 4`. The nonlinear equality has multiplier `3.2426`, which equals `(3 - 1/sqrt 2) * sqrt 2`, the gradient magnitude divided by the constraint-gradient magnitude. A family the problem does not use has an empty multiplier series; an inactive constraint has multiplier zero.

### Program 11: the one-line and many-line summaries

Two ready-made reports describe a finished run. `compact_summary` (also reachable as `print_summary`) is one line for logs. `detailed_summary` is one field per line.

```cpp
// Three ready-made reports for a finished run. compact_summary (and print_summary, its alias) is one
// line for logs. detailed_summary is one field per line for a terminal.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto r = optim::optima([](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); },
                               vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));

  std::cout << "-- compact_summary --\n";
  optim::reports::compact_summary(std::cout, r) << "\n";
  std::cout << "-- print_summary (same line) --\n";
  optim::print_summary(std::cout, r) << "\n";
  std::cout << "-- detailed_summary --\n";
  optim::reports::detailed_summary(std::cout, r) << "\n";
  return 0;
}
```

### Output

```text
-- compact_summary --
algorithm=sqp exit=1 iterations=1 evaluations=2 objective=1.25 violation=0 first_order=2e-08 backend_parameter=10 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
-- print_summary (same line) --
algorithm=sqp exit=1 iterations=1 evaluations=2 objective=1.25 violation=0 first_order=2e-08 backend_parameter=10 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
-- detailed_summary --
algorithm=sqp
exit=1
iterations=1
evaluations=2
constraint_violation=0
first_order_optimality=2e-08
step_size=1
backend_parameter=10
complementarity=0
krylov_iterations=0
best_feasible_available=true
returned_best_feasible=true
stall=none
stall_measure=0
stall_reference=0
message=optimality and feasibility tolerances satisfied
```
Both print the same facts. The compact form is the one to put in a log line; the detailed form is for a terminal, and its field names match the `output` table above.

## Decisions

- **What to branch on.** Branch on `exit_flag`. Use `converged` as a shorthand for "the flag is `Converged`". Treat the message as text for people.
- **What to do after a failure.** If the flag is `MaxIterations` or `Stopped`, read `output.best_feasible_available`; the checkpoint may be a usable answer. If it is `NoProgress`, `NoFeasiblePoint` or `ConstraintStationary`, suspect the problem (infeasible constraints, a bad start) before the solver settings.
- **How much progress output.** `reports::iter` for a watched run. A custom hook for a monitor, a stop rule, or a record. Nothing for production.

## Pitfalls

### Using the answer without checking the flag

The result always holds a point. After a failure that point is just wherever the solver ended. Using it as if it were a solution is the most common mistake in code that calls a solver.

```cpp
// Pitfall: every result carries an answer, even when the solver failed. After an impossible problem
// the answer is just the last point reached. Reading result.params without checking the exit flag
// treats a failure as a solution. Check exit_flag (or converged) first, and use the best feasible
// checkpoint only when output.best_feasible_available says one exists.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  // x0^2 + x1^2 <= -1 has no solution.
  const auto impossible = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, impossible);

  const auto x = r.params.at("x").to_vec<double>();
  std::cout << "unchecked answer: [" << x[0] << ", " << x[1] << "]  (looks like a result)\n";
  std::cout << "converged: " << std::boolalpha << r.converged << ", exit flag " << static_cast<int>(r.exit_flag)
            << ", violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible point exists: " << r.output.best_feasible_available << "\n";
  if (r.exit_flag != optim::OptimaExitFlag::Converged) {
    std::cout << "decision: do not use this answer\n";
  }
  return 0;
}
```

### Output

```text
unchecked answer: [0, 0]  (looks like a result)
converged: false, exit flag -6, violation 1
best feasible point exists: false
decision: do not use this answer
```
The impossible problem returns the start point, `[0, 0]`, which looks like a perfectly good answer. Only the flag (`-6`) and the violation (`1`) reveal that it is not. Check the flag before reading `params`.

## Where next

The next chapter compares the four algorithms: which problems each one handles well, where each one fails, and how to choose between them.
