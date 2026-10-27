<!-- generated from simulation_as_a_check.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulation as a check

[Back to State-Space Models](../ts_05_state_space_models.md)

A state-space model is a recipe for generating data, so it can be run forwards as well as fitted backwards. `Ssm::simulate(T, model, num_paths, cfg, seed)` draws `num_paths` independent paths of `T` observations (and the underlying states) from the model, starting from the distribution of the initial state (`mean0`, `cov0`). With more than one path the arrays gain a leading path dimension: `observations` has shape `[paths, T, n]`. The result also holds the drawn disturbances (`state_disturbance`, `obs_innovation`).

That makes simulation useful in three ways, and each of the programs below is one of them:

1. **A unit test of the model.** If you enter the matrices wrongly (variances where standard deviations belong, a transposed `C`) the simulated moments will not match what the model predicts analytically. It is the quickest way to find a mistake in a new model.
2. **A test of an estimator.** Generate data from known parameters, estimate them, and see how far the estimates land from the truth. It shows how much data a parameter needs.
3. **An answer to a question the formulas do not give.** The Kalman forecast gives the mean and variance of a future value; it does not give the probability that the series *crosses a level at any time* in the next weeks. Simulation does.

## Does the model do what I think it does?

The first program builds a local level model (`x_{t+1} = x_t + 0.5 w_t`, `y_t = x_t + 0.3 v_t`, initial state `N(0, 1)`), simulates 20 000 paths, and compares the *sample* variance of the observations at each time with the variance the model itself predicts (`Ssm::forecast(h, model)` with no data). For this model the predicted variance can also be written down: the state variance starts at `1` and gains `0.5² = 0.25` per step, and the observation adds `0.3² = 0.09`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }

int main() {
  ts::SsmModel m;
  m.transition = matrix({1}, {1, 1});
  m.state_loading = matrix({0.5}, {1, 1});
  m.observation = matrix({1}, {1, 1});
  m.innovation_loading = matrix({0.3}, {1, 1});
  m.mean0 = matrix({0}, {1});
  m.cov0 = matrix({1}, {1, 1});

  const std::int64_t horizon = 6, paths = 20000;
  const auto sim = ts::Ssm::simulate(horizon, m, paths, {}, /*seed=*/20261022);
  const auto predicted = ts::Ssm::forecast(horizon, m);
  std::cout << "observations: " << sim.observations.sizes()[0] << " paths x " << sim.observations.sizes()[1] << " times x " << sim.observations.sizes()[2] << "\n";
  std::cout << std::fixed << std::setprecision(3) << std::setw(6) << "time" << std::setw(14) << "simulated" << std::setw(14) << "model" << std::setw(14) << "by hand" << "\n";
  for (std::int64_t t = 0; t < horizon; ++t) {
    const Series y_t = sim.observations.select(1, t).reshape({paths});
    const double mean = y_t.mean().item<double>();
    const double variance = (y_t * y_t).mean().item<double>() - mean * mean;
    const double by_hand = 1.0 + 0.25 * (t + 1) + 0.09;
    std::cout << std::setw(6) << t << std::setw(14) << variance << std::setw(14) << predicted.observations_cov.select(0, t).select(0, 0).select(0, 0).item<double>() << std::setw(14) << by_hand << "\n";
  }
}
```

Output

```text
observations: 20000 paths x 6 times x 1
  time     simulated         model       by hand
     0         1.315         1.340         1.340
     1         1.566         1.590         1.590
     2         1.832         1.840         1.840
     3         2.076         2.090         2.090
     4         2.338         2.340         2.340
     5         2.586         2.590         2.590
```

The simulated variance agrees with the model's own prediction and with the hand calculation up to simulation error (a few hundredths with 20 000 paths). If a column disagrees, the model has been mis-specified or mis-entered; and note which one disagrees. A mismatch between the model and the hand calculation means the *model* is not what you meant; a mismatch between the simulation and the model means a bug in the code.

## How well can the noise be estimated?

The second program asks how precisely the observation noise can be recovered from data. It simulates 30 independent series of a given length from a local level model with `σ_v = 0.5` and `σ_w = 0.2`, and for each estimates `σ_v` by maximising `loglik` over a grid (the level noise is held at its true value). It repeats this for series of 50 and 200 observations and reports the mean and spread of the estimates.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }

ts::SsmModel local_level(double level_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = matrix({1}, {1, 1});
  m.state_loading = matrix({level_sd}, {1, 1});
  m.observation = matrix({1}, {1, 1});
  m.innovation_loading = matrix({obs_sd}, {1, 1});
  m.mean0 = matrix({0}, {1});
  m.cov0 = matrix({25}, {1, 1});
  return m;
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto truth = local_level(0.2, 0.5);
  std::vector<double> grid;
  for (double s = 0.2; s <= 0.9001; s += 0.05) grid.push_back(s);

  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::setw(8) << "T" << std::setw(18) << "mean estimate" << std::setw(12) << "std dev" << "   (true observation sd 0.500)\n";
  for (int n : {50, 200}) {
    std::vector<double> est;
    for (int r = 0; r < 30; ++r) {
      const Series y = ts::Ssm::simulate(n, truth, 1, f64, /*seed=*/1000 + r).observations.reshape({n, 1});
      double best = -1e300, best_s = 0.0;
      for (double s : grid) {
        const double ll = ts::Ssm::filter(y, local_level(0.2, s)).loglik;
        if (ll > best) { best = ll; best_s = s; }
      }
      est.push_back(best_s);
    }
    double m = 0.0, v = 0.0;
    for (double e : est) m += e;
    m /= static_cast<double>(est.size());
    for (double e : est) v += (e - m) * (e - m);
    std::cout << std::setw(8) << n << std::setw(18) << m << std::setw(12) << std::sqrt(v / (est.size() - 1)) << "\n";
  }
}
```

Output

```text
       T     mean estimate     std dev   (true observation sd 0.500)
      50             0.488       0.034
     200             0.480       0.025
```

On average the estimate is close to the truth (0.488 and 0.480 against 0.5), and the spread across samples is 0.034 at 50 observations and 0.025 at 200: about 7% and 5% of the value. Two things limit the gain from more data here: the grid step of 0.05 puts a floor under the resolution, and in a local level model the observation noise is separated from the level noise only through the dynamics, which needs a long record. That is the kind of fact a simulation reveals and a single fit cannot. The same procedure, repeated for every parameter of interest, is how to decide whether a series is long enough to estimate a model: if the spread at your sample size is larger than the differences you care about, fix the parameter or collect more data.

## A question the forecast cannot answer

The Kalman forecast gives the distribution of the series at each future date separately. It cannot say how likely the series is to *cross a level at some point before a date*, which depends on the whole path. To answer that, simulate paths from the *current* state: set `mean0` and `cov0` of a copy of the model to the filtered state at the last observation, and simulate forward. The program does this for a local level series, finds the probability that the level exceeds a threshold at the end of 20 steps (which the Gaussian forecast also gives, as a check) and the probability that it exceeds it *at any time* in the 20 steps.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }

ts::SsmModel local_level(double level_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = matrix({1}, {1, 1});
  m.state_loading = matrix({level_sd}, {1, 1});
  m.observation = matrix({1}, {1, 1});
  m.innovation_loading = matrix({obs_sd}, {1, 1});
  m.mean0 = matrix({0}, {1});
  m.cov0 = matrix({25}, {1, 1});
  return m;
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  auto model = local_level(0.3, 0.5);
  const int n = 100;
  const Series y = ts::Ssm::simulate(n, model, 1, f64, /*seed=*/6).observations.reshape({n, 1});

  const auto fit = ts::Ssm::filter(y, model);
  const double last = y.select(0, n - 1).item<double>();
  const double threshold = last + 1.0;  // one unit above the last observation

  // The state after the last observation, as the starting point of the simulation.
  auto future = model;
  future.mean0 = fit.filtered_states.select(0, n - 1).reshape({1});
  future.cov0 = fit.filtered_states_cov.select(0, n - 1).reshape({1, 1});

  const int horizon = 20, paths = 20000;
  const auto sim = ts::Ssm::simulate(horizon, future, paths, f64, /*seed=*/99);
  const auto obs = sim.observations.reshape({paths, horizon}).host<double>();
  int end_above = 0, ever_above = 0;
  for (int p = 0; p < paths; ++p) {
    bool ever = false;
    for (int t = 0; t < horizon; ++t) ever = ever || obs[p * horizon + t] > threshold;
    ever_above += ever;
    end_above += obs[p * horizon + horizon - 1] > threshold;
  }

  const auto f = ts::Ssm::forecast(horizon, y, model);
  const double mean = f.observations.select(0, horizon - 1).item<double>();
  const double sd = std::sqrt(f.observations_cov.select(0, horizon - 1).select(0, 0).select(0, 0).item<double>());
  const double gaussian = 0.5 * std::erfc((threshold - mean) / (sd * std::sqrt(2.0)));

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "last observation " << last << ", threshold " << threshold << "\n";
  std::cout << "P(above the threshold at step 20), simulated:        " << static_cast<double>(end_above) / paths << "\n";
  std::cout << "P(above the threshold at step 20), Gaussian forecast: " << gaussian << "\n";
  std::cout << "P(above the threshold at some step in 1..20), simulated: " << static_cast<double>(ever_above) / paths << "\n";
}
```

Output

```text
last observation -3.095, threshold -2.095
P(above the threshold at step 20), simulated:        0.433
P(above the threshold at step 20), Gaussian forecast: 0.434
P(above the threshold at some step in 1..20), simulated: 0.892
```

The first two numbers agree (0.433 simulated against 0.434 from the Gaussian forecast), which validates the simulation set-up: the Gaussian forecast and the simulated distribution at step 20 are the same thing. The third number is much larger, and it is the one the forecast cannot provide: the probability that the series *touches* the threshold at some point in the next 20 steps is 0.89, about twice the probability of being above it at the end, because a path can cross and come back. Any question about a path rather than a single date (the largest value, the time to cross a level, the number of days above a barrier) is a simulation question.

## Decisions and pitfalls

- **Simulate before you fit.** A new model that cannot reproduce its own moments has a coding error; this costs seconds.
- **Use enough paths.** The error of a Monte Carlo probability falls like `1/√paths`; 20 000 paths gives about ±0.003 near a probability of 0.5, and much more is needed for small probabilities.
- **Fix the seed for reproducibility**, and vary it to see Monte Carlo noise.
- **Simulating the future needs the right starting state.** Set `mean0` and `cov0` to the filtered state at the end of the data (as above) or the paths will begin from the prior, not from where the series is.
- **Simulation reuses the fitted noise levels.** If those are estimates, the answers inherit their error; simulate over the estimation uncertainty too when it matters.

Where next: [Chapter 6](../ts_06_direct_forecasting.md) forecasts without a model of the data-generating process, by learning each horizon directly.
