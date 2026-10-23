<!-- generated from simulation_as_a_check.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulation as a check

[Back to State-Space Models](../ts_05_state_space_models.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(f64); }

int main() {
  ts::SsmModel m;
  m.transition = matrix({1}, {1, 1});
  m.state_loading = matrix({0.5}, {1, 1});
  m.observation = matrix({1}, {1, 1});
  m.innovation_loading = matrix({0.3}, {1, 1});
  m.mean0 = matrix({0}, {1});
  m.cov0 = matrix({1}, {1, 1});

  const std::int64_t horizon = 5, paths = 20000;
  const auto sim = ts::Ssm::simulate(horizon, m, paths, {}, 20261022);
  const auto predicted = ts::Ssm::forecast(horizon, m);
  std::cout << "observations: " << sim.observations.sizes()[0] << " paths x " << sim.observations.sizes()[1] << " times x " << sim.observations.sizes()[2] << "\n";
  std::cout << std::fixed << std::setprecision(3) << std::setw(6) << "time" << std::setw(14) << "simulated" << std::setw(14) << "predicted" << "\n";
  for (std::int64_t t = 0; t < horizon; ++t) {
    const Series y_t = sim.observations.select(1, t).reshape({paths});
    const double variance = (y_t * y_t).mean().item<double>() - y_t.mean().item<double>() * y_t.mean().item<double>();
    std::cout << std::setw(6) << t << std::setw(14) << variance << std::setw(14) << predicted.observations_cov.select(0, t).select(0, 0).select(0, 0).item<double>() << "\n";
  }
}
```

Output

```text
observations: 20000 paths x 5 times x 1
  time     simulated     predicted
     0         1.341         1.340
     1         1.601         1.590
     2         1.871         1.840
     3         2.116         2.090
     4         2.352         2.340
```
The variance across 20 000 simulated paths agrees with the variance the model predicts analytically (1.34, 1.59, 1.84, … rising by `B B'` = 0.25 each step) up to simulation error. Use this check whenever you set up a new model: a mistake in a matrix shows up as a mismatch.
