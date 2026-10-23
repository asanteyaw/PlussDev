// Simulating a state-space model. `Ssm::simulate(T, model, num_paths, cfg, seed)` draws paths of states and observations from the model, starting from the initial
// state distribution (`mean0`, `cov0`); with `num_paths > 1` the arrays gain a leading path dimension ([P, T, n]). The result also holds the drawn disturbances. As a check,
// the variance of the observations across many paths is compared with the variance that the model itself predicts (`Ssm::forecast(h, model)` with no data), for a local
// level model: x_{t+1} = x_t + 0.5 w_t, y_t = x_t + 0.3 v_t, with initial state N(0, 1).
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
