<!-- generated from filtering_a_local_linear_trend.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Filtering a local linear trend

[Back to State-Space Models](../ts_05_state_space_models.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(f64); }
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + 1.0 e_t, t = 0 ... n - 1.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(static_cast<std::size_t>(n));
  for (int t = 0; t < n; ++t) y[static_cast<std::size_t>(t)] = 1.0 + 0.3 * t + e[static_cast<std::size_t>(t)];
  return y;
}
// Local linear trend: the state is (level, slope); level_{t+1} = level_t + slope_t + level_sd w1, slope_{t+1} = slope_t + slope_sd w2, and y_t = level_t + obs_sd v.
ts::SsmModel local_linear_trend(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = matrix({1, 1, 0, 1}, {2, 2});
  m.state_loading = matrix({level_sd, 0, 0, slope_sd}, {2, 2});
  m.observation = matrix({1, 0}, {1, 2});
  m.innovation_loading = matrix({obs_sd}, {1, 1});
  m.mean0 = matrix({0, 0}, {2});
  m.cov0 = matrix({25, 0, 0, 1}, {2, 2});
  return m;
}
int main() {
  const int n = 60;
  const Series y = matrix(trending(n), {n, 1});
  std::cout << std::fixed << std::setprecision(3);
  const auto fit = ts::Ssm::filter(y, local_linear_trend(0.2, 0.05, 0.3));
  std::cout << "filtered level and slope, and one-step forecast error\n";
  for (int t : {0, 1, 2, 10, 30, 59}) {
    const auto x = fit.filtered_states.select(0, t).to_vec<double>();
    std::cout << "  t = " << std::setw(2) << t << "   level " << std::setw(7) << x[0] << "   slope " << std::setw(6) << x[1] << "   error " << std::setw(7)
              << fit.forecast_obs_error.select(0, t).item<double>() << "\n";
  }
  std::cout << "log-likelihood " << fit.loglik << "\n\nobservation noise scan (level sd 0.2, slope sd 0.05)\n";
  for (double sd : {0.05, 0.1, 0.2, 0.3, 0.4, 0.6, 1.0})
    std::cout << "  obs sd " << std::setw(5) << sd << "   log-likelihood " << std::setw(8) << ts::Ssm::filter(y, local_linear_trend(0.2, 0.05, sd)).loglik << "\n";
}
```

Output

```text
filtered level and slope, and one-step forecast error
  t =  0   level   1.236   slope  0.047   error   1.240
  t =  1   level   1.358   slope  0.113   error   0.081
  t =  2   level   1.541   slope  0.152   error   0.083
  t = 10   level   3.909   slope  0.277   error   0.445
  t = 30   level   9.959   slope  0.292   error   0.251
  t = 59   level  18.738   slope  0.307   error  -0.058
log-likelihood -32.005

observation noise scan (level sd 0.2, slope sd 0.05)
  obs sd 0.050   log-likelihood  -79.308
  obs sd 0.100   log-likelihood  -55.179
  obs sd 0.200   log-likelihood  -33.231
  obs sd 0.300   log-likelihood  -32.005
  obs sd 0.400   log-likelihood  -36.827
  obs sd 0.600   log-likelihood  -49.608
  obs sd 1.000   log-likelihood  -71.894
```
The local linear trend model has a level and a slope; the level moves by the slope each step, and both are disturbed by small shocks. Started from an uninformative prior, the filter learns the slope quickly: it is 0.05 after the first observation, 0.15 after three and about 0.3 from then on (the data rise 0.3 per step). The forecast errors shrink from 1.24 (the prior is far from the first value) to values near the observation noise. The scan over the observation noise standard deviation has its peak at 0.3, next to the value 0.289 that generated the data; too small a value makes the filter chase noise (the likelihood falls to −79), too large a value makes it sluggish.
