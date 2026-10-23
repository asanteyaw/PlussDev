<!-- generated from smoothing_across_a_gap_and_forecasting.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Smoothing across a gap, and forecasting

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
  const int n = 40;
  auto data = trending(n);
  for (int t = 15; t < 20; ++t) data[static_cast<std::size_t>(t)] = std::numeric_limits<double>::quiet_NaN();
  const Series y = matrix(data, {n, 1});
  const auto model = local_linear_trend(0.2, 0.05, 0.3);
  const auto s = ts::Ssm::smooth(y, model);
  std::cout << std::fixed << std::setprecision(3) << "around the gap (t = 15 ... 19 missing; true level 1 + 0.3 t)\n";
  std::cout << std::setw(4) << "t" << std::setw(8) << "used" << std::setw(11) << "filtered" << std::setw(11) << "smoothed" << std::setw(14) << "smoothed sd" << std::setw(8) << "truth" << "\n";
  for (int t = 12; t < 23; ++t) {
    const double used = s.filter.data_used.select(0, t).item<double>();
    const double sd = std::sqrt(s.smoothed_states_cov.select(0, t).select(0, 0).select(0, 0).item<double>());
    std::cout << std::setw(4) << t << std::setw(8) << (used > 0 ? "yes" : "no") << std::setw(11) << s.filter.filtered_states.select(0, t).select(0, 0).item<double>() << std::setw(11)
              << s.smoothed_states.select(0, t).select(0, 0).item<double>() << std::setw(14) << sd << std::setw(8) << 1.0 + 0.3 * t << "\n";
  }
  const auto f = ts::Ssm::forecast(5, y, model);
  std::cout << "\nforecast of the next five observations (truth 1 + 0.3 t)\n";
  for (int h = 0; h < 5; ++h)
    std::cout << "  t = " << n + h << "   " << std::setw(7) << f.observations.select(0, h).item<double>() << "  +/- " << std::setw(6)
              << 1.96 * std::sqrt(f.observations_cov.select(0, h).select(0, 0).select(0, 0).item<double>()) << "   truth " << std::setw(6) << 1.0 + 0.3 * (n + h) << "\n";
}
```

Output

```text
around the gap (t = 15 ... 19 missing; true level 1 + 0.3 t)
   t    used   filtered   smoothed   smoothed sd   truth
  12     yes      4.551      4.622         0.174   4.600
  13     yes      5.078      4.980         0.180   4.900
  14     yes      5.196      5.215         0.202   5.200
  15      no      5.489      5.528         0.267   5.500
  16      no      5.783      5.842         0.302   5.800
  17      no      6.076      6.155         0.313   6.100
  18      no      6.369      6.468         0.302   6.400
  19      no      6.663      6.779         0.267   6.700
  20     yes      7.320      7.087         0.202   7.000
  21     yes      7.140      7.275         0.180   7.300
  22     yes      7.593      7.658         0.174   7.600

forecast of the next five observations (truth 1 + 0.3 t)
  t = 40    13.116  +/-  0.909   truth 13.000
  t = 41    13.439  +/-  1.105   truth 13.300
  t = 42    13.762  +/-  1.321   truth 13.600
  t = 43    14.085  +/-  1.557   truth 13.900
  t = 44    14.408  +/-  1.809   truth 14.200
```
Five observations are missing (`used` is "no"). Inside the gap the filtered level simply extrapolates the trend, while the smoothed level also uses the data after the gap and ends up closer to the truth (6.779 against 6.663 at time 19, truth 6.700). The smoothed standard deviation rises in the middle of the gap (0.31) and returns to 0.17 where data are dense: the smoother reports honestly how little it knows where data are missing. The forecast carries the trend forward with intervals that widen with the horizon, because both the level and the slope are uncertain.
