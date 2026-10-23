<!-- generated from writing_your_own_hook.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Writing your own hook

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


A hook is any function from a snapshot to a bool. The snapshot carries the stage, the current point, the full evaluation, and the work counters. This program records the objective at every iteration and stops the run as soon as the point is feasible and good enough.

```cpp
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
