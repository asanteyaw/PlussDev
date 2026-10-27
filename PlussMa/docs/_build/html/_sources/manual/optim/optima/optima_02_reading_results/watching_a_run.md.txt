<!-- generated from watching_a_run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Watching a run

[Back to Optima 2: Reading Results and Watching a Run](../optima_02_reading_results.md)

Everything on the previous pages is available *after* a run. A **progress hook** gives it to you *during* the run, one snapshot per iteration, and it lets you stop the run yourself. A hook is any function from a snapshot to a bool: the snapshot carries the stage (`start`, `iter` or `done`), the current point, the full evaluation (value, gradient, constraint violation) and the work counters, and returning `true` stops the run with the flag `Stopped`.

`reports::trace` and `reports::iter` turn an output stream into a ready-made hook that prints one line per iteration, which is the quickest way to watch a run. Writing your own is a few lines and doubles as a monitor (log the objective, plot the violation, stop when the point is good enough). The last program is about precedence: you can set one hook that applies to whichever algorithm runs, or a hook on one algorithm's own options, and when both are set the specific one wins.

## Program 6: Printing progress

`reports::trace` turns any output stream into a progress hook that prints one line per iteration. `reports::iter` is the short form. `ProgressTracePreset` switches each part of a line on or off.

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
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

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

Output

```text
-- reports::trace, default preset --
iter=0 evals=1 objective=18.000000 viol=0.000000 stationarity=6.000000 backend=10.000000 step=0.000000 krylov=0 stall=none msg="initialized"
iter=1 evals=6 objective=10.125000 viol=0.125000 stationarity=1.000000 backend=5.000000 step=0.125000 krylov=0 stall=none msg="step accepted"
iter=2 evals=7 objective=10.503472 viol=0.003472 stationarity=0.013889 backend=3.371111 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=3 evals=8 objective=10.514709 viol=0.000003 stationarity=0.001657 backend=3.576542 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000002 backend=3.576905 step=1.000000 krylov=0 stall=none msg="step accepted"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000000 backend=3.576905 step=1.000000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
iter=4 evals=9 objective=10.514719 viol=0.000000 stationarity=0.000000 backend=3.576905 step=1.000000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
-- reports::iter, three decimals --
iter=1 evals=6 objective=10.125 viol=0.125 stationarity=1.000 backend=5.000 step=0.125 stall=none
iter=2 evals=7 objective=10.503 viol=0.003 stationarity=0.014 backend=3.371 step=1.000 stall=none
iter=3 evals=8 objective=10.515 viol=0.000 stationarity=0.002 backend=3.577 step=1.000 stall=none
iter=4 evals=9 objective=10.515 viol=0.000 stationarity=0.000 backend=3.577 step=1.000 stall=none
iter=4 evals=9 objective=10.515 viol=0.000 stationarity=0.000 backend=3.577 step=1.000 stall=none
-- a custom preset: no start line, no final line, no message, no Krylov count --
iter=1 evals=6 objective=10.1250 viol=0.1250 stationarity=1.0000 backend=5.0000 step=0.1250 stall=none
iter=2 evals=7 objective=10.5035 viol=0.0035 stationarity=0.0139 backend=3.3711 step=1.0000 stall=none
iter=3 evals=8 objective=10.5147 viol=0.0000 stationarity=0.0017 backend=3.5765 step=1.0000 stall=none
iter=4 evals=9 objective=10.5147 viol=0.0000 stationarity=0.0000 backend=3.5769 step=1.0000 stall=none
iter=4 evals=9 objective=10.5147 viol=0.0000 stationarity=0.0000 backend=3.5769 step=1.0000 stall=none
```
The default trace prints a start line, one line per iteration, and a final line, each with the objective, violation, stationarity, backend parameter, step and message. `reports::iter` drops the start and final lines, the Krylov count and the message, and takes the number of decimals. It also takes an optional third argument, `every`, to print only every n-th iteration. The same `reports::iter` is the progress hook of `septima` too: the hook object can be called with either solver's progress snapshot, and the solver you give it to decides which one it receives. For an `optima` run each line carries the evaluations, objective, violation, stationarity, step and stall category; for a `septima` run each line carries the objective, the largest gradient entry and the largest step entry (see [Reading and watching a run](../../septima/septima_01_first_problems/reading_a_run.md)). The custom preset does the same with four decimals. Notice the two lines that repeat at the end of the first trace: the last iteration is reported once as an iteration and once as `done`.

Read a trace by watching two columns. A run that is working shows `viol` falling to zero and `stationarity` falling at the end. Here both fall by orders of magnitude on every iteration, `viol` from `0.125` to `0.0035` to `3e-06` to zero and `stationarity` from `1` to `0.019` to `0.002` to `2e-06`: the signature of a quadratic-model method closing in on a solution. A trace that instead jumps back up, or stalls, is the signal to look at the stall category.

### One name for two solvers

`reports::iter` is not a plain function value. It returns a small object, `reports::IterReport`, that has one call operator for each solver's progress snapshot:

```cpp
bool operator()(const OptimaProgressSnapshot&) const;   // evaluations, objective, violation, stationarity, step, stall
bool operator()(const SeptimaProgressSnapshot&) const;  // objective, largest gradient entry, largest step entry
```

The two solvers store their hook as a `std::function` over their own snapshot type: `OptimaOptions::progress_hook` takes an `OptimaProgressSnapshot` and `SeptimaSolveOptions::progress_hook` takes a `SeptimaProgressSnapshot`. When the object is assigned to either one, the compiler selects the call operator whose parameter matches that snapshot, so the same `reports::iter(std::cout, 4)` prints optima columns in one place and septima columns in the other. Nothing is decided while the program runs and nothing needs to be configured.

The same trick works for a hook of your own: a generic lambda such as `[](const auto& snapshot) { std::cout << snapshot.iteration << "\n"; return false; }` can be assigned to either solver, because both snapshots have an `iteration` member. A solver whose snapshot the object has no call operator for will not compile, which is the safe failure: the mismatch is reported when you build, not in the middle of a run.


## Program 7: Writing your own hook

A hook is any function from a snapshot to a bool. The snapshot carries the stage, the current point, the full evaluation, and the work counters. This program records the objective at every iteration and stops the run as soon as the point is feasible and good enough.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

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
  const auto x = r.params.at("x").host<double>();
  std::cout << "returned point [" << x[0] << ", " << x[1] << "]\n";
  return 0;
}
```

Output

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

## Program 8: Which hook runs

You can set one hook on `OptimaOptions::progress_hook` and have it apply to whichever algorithm you choose, or set a hook on one algorithm's own options. When both are set the specific one wins.

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
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };

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

Output

```text
general hook only:      general=4 specific=0
both hooks set:         general=0 specific=4
other algorithm chosen: general=4 specific=0
```
With only the general hook set it received all four snapshots. After adding a sequential QP hook, the specific hook received them and the general one received none. Choosing a different algorithm falls back to the general hook, because the sequential QP hook belongs to a method that is not running.

## Decisions and pitfalls

- **Read a trace by two columns.** A working run shows the violation falling to zero and the stationarity falling at the end; a trace that jumps back up, or stalls, is the signal to look at the stall category.
- **`done` fires even when the hook ends the run.** The final snapshot is delivered once, so code that counts iterations should not count it.
- **A hook that does heavy work slows every iteration.** Keep it light, or sample (act on every tenth `iter`).
- **A specific hook replaces the general one.** If a method's own hook is set, the general hook receives nothing for that method; set one or the other, not both, unless you mean it.
- **Stopping from a hook is a decision about quality.** Stop on a condition you can state (feasible and below a value), and check the flag afterwards: the result is `Stopped`, not `Converged`.

Where next: [Chapter 3](../optima_03_choosing_an_algorithm.md) compares the four algorithms and how to choose between them.
