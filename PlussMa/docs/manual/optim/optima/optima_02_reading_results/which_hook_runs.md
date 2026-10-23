<!-- generated from which_hook_runs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Which hook runs

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


You can set one hook on `OptimaOptions::progress_hook` and have it apply to whichever algorithm you choose, or set a hook on one algorithm's own options. When both are set the specific one wins.

```cpp
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

Output

```text
general hook only:      general=4 specific=0
both hooks set:         general=0 specific=4
other algorithm chosen: general=4 specific=0
```
With only the general hook set it received all four snapshots. After adding a sequential QP hook, the specific hook received them and the general one received none. Choosing a different algorithm falls back to the general hook, because the sequential QP hook belongs to a method that is not running.
