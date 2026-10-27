<!-- generated from exit_flags_and_stall_categories.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Exit flags and stall categories

[Back to Optima 2: Reading Results and Watching a Run](../optima_02_reading_results.md)

A solver run can end in many ways and only one of them means "here is your answer". The first thing to learn is to branch on the right thing. Every call returns four independent pieces of information, and this page is about the first two and the diagnosis that goes with them: the **exit flag**, the verdict (one of nine values, and the thing to branch on); `result.converged`, a convenience that is true exactly when the flag is `Converged`; the **stall category**, which names the kind of difficulty behind a stop that was not a success; and a human-readable message that is for logs and must never be branched on.

The programs run all four algorithms on an easy problem to show they share one result type, then trigger each healthy flag and each failure flag on purpose, then each of the ten stall categories. Reading them in order builds the habit that matters most in code that calls a solver: *check the flag before you trust the point*, because the result always holds a point and after a failure it is just wherever the solver ended.

## Program 1: The four algorithms side by side

Choosing an algorithm is the subject of the next chapter. For now, the first program shows that all four share one result type, so every report in this chapter works the same for each. It also shows how little they differ in what they return on an easy problem, and how much in the work they do to get there.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };
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
    const auto x = r.params.at("x").host<double>();
    std::cout << std::left << std::setw(17) << name << std::setw(24) << ("label=" + r.output.algorithm) << std::right
              << "x=[" << x[0] << ", " << x[1] << "]  flag=" << std::setw(2) << static_cast<int>(r.exit_flag)
              << "  iterations=" << std::setw(3) << r.iteration << "  evaluations=" << std::setw(3)
              << r.function_evaluations << "\n";
  }
  return 0;
}
```

Output

```text
MeritMethod      label=merit-method      x=[1.5000, 0.8000]  flag= 1  iterations=  2  evaluations=  3
SequentialQP     label=sqp               x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
WorkingSet       label=working-set       x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
InternalProtocol label=internal-protocol x=[1.5000, 0.8000]  flag= 1  iterations=  5  evaluations=  8
```
All four agree on the answer, the corner `(1.5, 0.8)`, and all four exit with `Converged`. They differ in effort: the merit method needs two iterations and three evaluations because it handles bounds by projection, while sequential QP and the working-set method need a single iteration, because their first quadratic subproblem already identifies the corner, and the interior method takes five iterations and eight evaluations, since a barrier method approaches a bound from inside. `output.algorithm` carries each method's label.

## Program 2: The flags that mean a healthy run

Four flags describe a run that behaved as designed: it finished, it ran out of its budget, you stopped it, or it reached a target you set. This program triggers each one and prints the flag, the `converged` convenience, and the iteration count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  std::vector<std::pair<std::string, optim::OptimaState>> runs;

  // Converged: the usual outcome.
  runs.emplace_back("finished normally", optim::optima(objective, vec({0.0, 0.0})));

  // MaxIterations: the iteration budget is spent before the tolerances are met.
  optim::OptimaOptions few;
  few.sequential_qp.max_iterations = 1;
  runs.emplace_back("one iteration allowed", optim::optima(objective, vec({0.0, 0.0}), few, disc));

  // Stopped: the progress hook returned true, asking for an orderly stop.
  optim::OptimaOptions stop_early;
  int calls = 0;
  stop_early.progress_hook = [&](const optim::OptimaProgressSnapshot&) { return ++calls >= 2; };
  runs.emplace_back("hook asked to stop", optim::optima(objective, vec({0.0, 0.0}), stop_early));

  // ObjectiveTargetReached: a feasible point at or below objective_limit ends the run at once.
  optim::OptimaOptions good_enough;
  good_enough.sequential_qp.objective_limit = 100.0;
  runs.emplace_back("objective limit 100", optim::optima(objective, vec({0.0, 0.0}), good_enough));

  OutputEngine out;
  for (const auto& [label, r] : runs)
    out.writeln(label + ":", optim::optima_exit_flag_name(r.exit_flag), "code", static_cast<int>(r.exit_flag), "converged", r.converged ? "true" : "false", "iterations", r.iteration);
  return 0;
}
```

Output

```text
finished normally: Converged code 1 converged true iterations 1
one iteration allowed: MaxIterations code 0 converged false iterations 1
hook asked to stop: Stopped code -1 converged false iterations 1
objective limit 100: ObjectiveTargetReached code -3 converged false iterations 0
```
Only the first run is `converged`. The second spent its single allowed iteration, so the flag is `MaxIterations` and the iterate is not a solution. The third stopped because the hook returned `true` on its second call; `Stopped` always means the caller asked. The fourth ended at iteration zero because the start already satisfied `objective_limit = 100`. A feasible point at or below the limit ends the run at once with `ObjectiveTargetReached`.

## Program 3: The five failure flags

Five flags mean the solver could not deliver. Each is triggered here by a specific condition, and the result still carries the last point reached so you can inspect it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  std::vector<std::pair<std::string, optim::OptimaState>> runs;
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  // No point satisfies x0^2 + x1^2 + 1 <= 0, and none satisfies x0^2 + x1^2 + 1 = 0.
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  // NoFeasiblePoint: the interior method keeps bounds and linear rows strictly satisfied, so it first looks for
  // a point inside them. Here x0 + x1 <= -1 contradicts x >= 0, and it finds none.
  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  const auto impossible_linear = optim::constraints()
                                     .lbs(vec({0.0, 0.0}))
                                     .ln_ineq(pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(f64), vec({-1.0}));
  runs.emplace_back("interior method, impossible linear rows", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_linear));

  // ConstraintStationary also covers a nonlinear inequality that no point satisfies.
  runs.emplace_back("interior method, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), interior, impossible_inequality));

  // NoProgress: the recovery step could not reduce the violation, so the run gave up.
  runs.emplace_back("sequential QP, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  // ConstraintStationary: the violation stopped decreasing at a point that is still infeasible.
  runs.emplace_back("interior method, impossible equality", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  // NonFiniteEvaluation: the objective is not a number at the start (log of a negative value).
  runs.emplace_back("log of a negative start",
         optim::optima([](const pluss::Series& x) { return x.log().sum(); }, vec({-1.0, 1.0})));

  // InvalidProblem: the objective returned two values instead of one.
  runs.emplace_back("objective returns a vector", optim::optima([](const pluss::Series& x) { return x.square(); }, vec({1.0, 1.0})));

  OutputEngine out;
  for (const auto& [label, r] : runs) out.writeln(label + ":", optim::optima_exit_flag_name(r.exit_flag), "code", static_cast<int>(r.exit_flag), "max violation", r.evaluation.max_constraint_violation);
  return 0;
}
```

Output

```text
interior method, impossible linear rows: NoFeasiblePoint code -2 max violation 1
interior method, impossible inequality: ConstraintStationary code -7 max violation 1
sequential QP, impossible inequality: NoProgress code -6 max violation 1
interior method, impossible equality: ConstraintStationary code -7 max violation 1
log of a negative start: NonFiniteEvaluation code -4 max violation 0
objective returns a vector: InvalidProblem code -5 max violation 0
```
Impossible constraints produce different verdicts depending on their kind and the method. The interior method keeps bounds and linear rows strictly satisfied, so linear rows that contradict a bound (`x0 + x1 <= -1` with `x >= 0`) end the run before it starts with `NoFeasiblePoint`. A nonlinear inequality that no point satisfies, `x0^2 + x1^2 + 1 <= 0`, is different: the interior method gives it a slack variable and can start, so the verdict is `ConstraintStationary` once the violation can no longer be reduced (the gradient of its square vanishes), with the violation staying at `1`. Sequential QP tries to recover from the same inequality and fails, returning `NoProgress`. The impossible equality returns `ConstraintStationary` from the interior method for the same reason. A `log` of a negative start gives `NonFiniteEvaluation`, and an objective that returns a vector gives `InvalidProblem` with no exception thrown.

## Program 4: The ten stall categories

The exit flag is coarse. The stall category says what went wrong underneath. There are ten, and this program triggers each. The scenario text describes the cause, and the line below it shows the category name and the flag it came with.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using namespace pluss::tils;

int main() {
  std::vector<std::pair<std::string, optim::OptimaState>> runs;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;

  runs.emplace_back("a normal solve", optim::optima(objective, vec({0.0, 0.0})));

  optim::OptimaOptions hook;
  hook.progress_hook = [](const optim::OptimaProgressSnapshot&) { return true; };
  runs.emplace_back("the progress hook asks to stop", optim::optima(objective, vec({0.0, 0.0}), hook));

  runs.emplace_back("the objective returns a vector",
         optim::optima([](const pluss::Series& x) { return x.square(); }, vec({1.0, 1.0})));

  runs.emplace_back("the objective is not finite at the start",
         optim::optima([](const pluss::Series& x) { return x.log().sum(); }, vec({-1.0, 1.0})));

  optim::OptimaOptions merit;
  merit.algorithm = optim::OptimaAlgorithm::MeritMethod;
  runs.emplace_back("merit method on an impossible inequality", optim::optima(objective, vec({0.0, 0.0}), merit, impossible_inequality));

  optim::OptimaOptions no_step = interior;
  no_step.internal_protocol.fraction_to_boundary = 0.0;
  runs.emplace_back("no step is allowed toward the boundary", optim::optima(objective, vec({0.0, 0.0}), no_step, disc));

  runs.emplace_back("sequential QP cannot restore feasibility", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  runs.emplace_back("interior method stalls on an infeasible equality",
         optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  optim::OptimaOptions phase = interior;
  phase.internal_protocol.enable_feasibility_phase = true;
  runs.emplace_back("feasibility phase cannot reduce the violation", optim::optima(objective, vec({1.0, 1.0}), phase, impossible_equality));

  optim::OptimaOptions short_run = interior;
  short_run.internal_protocol.max_iterations = 2;
  runs.emplace_back("interior method stops with barrier complementarity unresolved",
         optim::optima(objective, vec({0.1, 0.1}), short_run, disc));

  optim::OptimaOptions one_iteration;
  one_iteration.sequential_qp.max_iterations = 1;
  runs.emplace_back("the iteration budget is spent", optim::optima(objective, vec({0.0, 0.0}), one_iteration, disc));

  OutputEngine out;
  for (const auto& [scenario, r] : runs) {
    out.writeln(scenario);
    out.writeln("    stall =", optim::optima_stall_category_name(r.output.stall_category), " flag =", static_cast<int>(r.exit_flag));
  }
  return 0;
}
```

Output

```text
a normal solve
    stall = none  flag = 1
the progress hook asks to stop
    stall = user-stop  flag = -1
the objective returns a vector
    stall = invalid-callback-contract  flag = -5
the objective is not finite at the start
    stall = nonfinite-callback-region  flag = -4
merit method on an impossible inequality
    stall = search-direction-collapse  flag = -6
no step is allowed toward the boundary
    stall = boundary-limited  flag = -6
sequential QP cannot restore feasibility
    stall = recovery-failure  flag = -6
interior method stalls on an infeasible equality
    stall = infeasible-stationary  flag = -7
feasibility phase cannot reduce the violation
    stall = feasibility-failure  flag = -6
interior method stops with barrier complementarity unresolved
    stall = complementarity-stagnation  flag = 0
the iteration budget is spent
    stall = iteration-limit  flag = 0
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

## Program 12: Using the answer without checking the flag

The result always holds a point. After a failure that point is just wherever the solver ended. Using it as if it were a solution is the most common mistake in code that calls a solver.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  // x0^2 + x1^2 <= -1 has no solution.
  const auto impossible = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, impossible);

  const auto x = r.params.at("x").host<double>();
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

Output

```text
unchecked answer: [0, 0]  (looks like a result)
converged: false, exit flag -6, violation 1
best feasible point exists: false
decision: do not use this answer
```
The impossible problem returns the start point, `[0, 0]`, which looks like a perfectly good answer. Only the flag (`-6`) and the violation (`1`) reveal that it is not. Check the flag before reading `params`.

## Decisions and pitfalls

- **Branch on `exit_flag` or `converged`, never on the message.** The wording of the message can change between versions; the flag is the contract.
- **`converged` is not the same as "the solver stopped".** `MaxIterations`, `Stopped` and `ObjectiveTargetReached` all stop the run and none is `converged`; each tells you something different about whether `params` is usable.
- **The stall category narrows a failure down; it never changes the verdict.** Use it to decide what to try next (a boxed problem that will not move points at `boundary-limited`; a run that spent its budget at `iteration-limit`).
- **Impossible constraints get different verdicts from different methods.** The interior method refuses a contradictory linear system outright (`NoFeasiblePoint`) but starts on a nonlinear inequality nobody can satisfy and ends `ConstraintStationary`. Test the verdict on your own problem.
- **After a failure `params` is a point, not a solution.** Always read the flag and the constraint violation before using it.

Where next: [The report, summaries and multipliers](the_report_summaries_and_multipliers.md) reads every field of the structured output.
