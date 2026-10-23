<!-- generated from printing_progress.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Printing progress

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


`reports::trace` turns any output stream into a progress hook that prints one line per iteration. `reports::iter` is the short form. `ProgressTracePreset` switches each part of a line on or off.

```cpp
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
The default trace prints a start line, one line per iteration, and a final line, each with the objective, violation, stationarity, backend parameter, step and message. `reports::iter` drops the start and final lines, the Krylov count and the message, and takes the number of decimals. The custom preset does the same with four decimals. Notice the two lines that repeat at the end of the first trace: the last iteration is reported once as an iteration and once as `done`.

Read a trace by watching two columns. A run that is working shows `viol` falling to zero and `stationarity` falling at the end. Here both fall by orders of magnitude on every iteration, `viol` from `0.125` to `0.0035` to `3e-06` to zero and `stationarity` from `1` to `0.019` to `0.002` to `2e-06`: the signature of a quadratic-model method closing in on a solution. A trace that instead jumps back up, or stalls, is the signal to look at the stall category.
