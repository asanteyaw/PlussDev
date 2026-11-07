<!-- generated from the_rank_of_a_cointegrated_system.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The rank of a cointegrated system

[Back to Vector Autoregressions and Cointegration](../ts_04_vector_models.md)

Two series that each wander without returning to a mean (each has a unit root) can still be *tied together*: a particular combination of them stays within a stable range and keeps returning to a mean. Spot and futures prices, two interest rates of different maturity, consumption and income are the textbook cases. Such series are **cointegrated**, and the combination, the long-run relation, is a stationary series built from non-stationary ones.

This matters for two reasons. First, a VAR in the *levels* of unit-root series is not safe, and a VAR in their *differences* throws the long-run relation away; the right model keeps both (the error-correction form of [the next page](the_error_correction_model.md)). Second, forecasts of cointegrated series stay together, where forecasts of unrelated unit-root series drift apart without limit.

A system of `k` series can have several independent long-run relations. Their number `r` is the **rank**: `r = 0` means no relation (the series are free to drift apart; difference them), `r = k` means every series is stationary (use a VAR in levels), and values in between mean `r` stationary combinations and `k − r` common stochastic trends. `johansen_test(y, JohansenOptions)` estimates the rank. It tests the sequence "rank ≤ 0", "rank ≤ 1", … and stops at the first hypothesis that is not rejected; that value is `selected_rank`. Two statistics are offered: the **trace** statistic tests "rank ≤ r" against any larger rank, and the **maximum eigenvalue** statistic tests "rank ≤ r" against exactly `r + 1`.

The reference distributions are *not* χ²: they are non-standard functionals of Brownian motion that depend on the deterministic terms in the model and on `k − r`. The library uses simulated limiting distributions for three specifications, selected by the options: no deterministic terms, an unrestricted constant (`include_intercept`, the default) and an unrestricted constant with a trend (`include_trend`). Pass the series in **levels**, and say honestly which deterministic terms the data have.

The three programs use simulated systems of three series in levels, where the true rank is known.

## The intuition: a stationary combination

Before the formal test, look at the idea. The first system has three series: `x` is a random walk, `y = x + u` with `u` a stationary AR(1) (so `y` is tied to `x`), and `w` is an unrelated random walk. The program runs the ADF test of [Chapter 1](../ts_01_describing_a_series/white_noise_unit_roots_stationarity.md) on each series and on three combinations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

// Columns: x (random walk), y = x + u (u an AR(1)), w (an unrelated random walk).
std::vector<std::vector<double>> three_series(int n, std::uint64_t seed, const pluss::Config& cfg) {
  ts::ArimaFit white;
  white.sigma = 0.5;
  ts::ArimaFit ar;
  ar.ar = {0.6};
  ar.sigma = 0.5;
  const auto x = white.simulate(n, cfg, seed).cumsum(0).host<double>();
  const auto u = ar.simulate(n, cfg, seed + 1).host<double>();
  const auto w = white.simulate(n, cfg, seed + 2).cumsum(0).host<double>();
  std::vector<double> y(n);
  for (int t = 0; t < n; ++t) y[t] = x[t] + u[t];
  return {x, y, w};
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto s = three_series(400, 20, f64);
  std::vector<double> y_minus_x(400), w_minus_x(400);
  for (int t = 0; t < 400; ++t) { y_minus_x[t] = s[1][t] - s[0][t]; w_minus_x[t] = s[2][t] - s[0][t]; }
  ts::AdfOptions o;
  o.lags = 1;
  const std::vector<std::pair<const char*, std::vector<double>>> cases = {{"x", s[0]}, {"y", s[1]}, {"w", s[2]}, {"y - x", y_minus_x}, {"w - x", w_minus_x}};
  for (const auto& [label, v] : cases) {
    const auto r = ts::adf_test(pluss::make_series(v, f64), o);
    out.writeln(label, "ADF statistic", r.stat, "p", r.pvalue, r.reject ? "stationary" : "unit root");
  }
}
```

Output

```text
x ADF statistic -0.567 p 0.878 unit root
y ADF statistic -1.113 p 0.710 unit root
w ADF statistic -2.580 p 0.097 unit root
y - x ADF statistic -9.339 p 0.000 stationary
w - x ADF statistic -0.867 p 0.799 unit root
```

The three series have unit roots (the ADF test cannot reject: p = 0.88, 0.71 and 0.10; the last is the closest to rejecting, as a test with a 5% size sometimes comes near), and so does the difference between the *unrelated* walks, `w − x` (p = 0.80). But `y − x` is stationary (statistic −9.3, p below 0.0005): the relation `y = x + noise` is the cointegrating relation, detected as a stationary series made from two non-stationary ones. The Johansen test does this systematically, without you guessing the combination.

## Testing for the rank

The program runs the test on the same system with both statistics. Each row tests "rank ≤ r"; a row that rejects means more relations are needed than `r`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

Series three_series(int n, std::uint64_t seed, const pluss::Config& cfg) {
  ts::ArimaFit white;
  white.sigma = 0.5;
  ts::ArimaFit ar;
  ar.ar = {0.6};
  ar.sigma = 0.5;
  const auto x = white.simulate(n, cfg, seed).cumsum(0).host<double>();
  const auto u = ar.simulate(n, cfg, seed + 1).host<double>();
  const auto w = white.simulate(n, cfg, seed + 2).cumsum(0).host<double>();
  std::vector<double> y(3 * n);
  for (int t = 0; t < n; ++t) { y[3 * t] = x[t]; y[3 * t + 1] = x[t] + u[t]; y[3 * t + 2] = w[t]; }
  return pluss::make_series(y, {n, 3}).to(pluss::ScalarType::Float64);
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series y = three_series(400, 20, f64);
  for (auto stat : {ts::JohansenStat::Trace, ts::JohansenStat::MaxEigen}) {
    ts::JohansenOptions o;
    o.lag_order = 1;
    o.statistic = stat;
    const auto r = ts::johansen_test(y, o, f64);
    const auto lambda = r.eigenvalues.host<double>(), s = r.stat.host<double>(), c = r.critical.host<double>(), p = r.pvalue.host<double>();
    const auto rej = r.reject.host<std::int64_t>();
    std::cout << (stat == ts::JohansenStat::Trace ? "trace statistic\n" : "maximum-eigenvalue statistic\n");
    std::cout << std::setw(12) << "H0: rank <=" << std::setw(12) << "eigenvalue" << std::setw(12) << "statistic" << std::setw(12) << "5% critical" << std::setw(10) << "p-value" << "\n";
    for (std::size_t i = 0; i < s.size(); ++i)
      std::cout << std::setw(12) << i << std::fixed << std::setprecision(4) << std::setw(12) << lambda[i] << std::setprecision(2) << std::setw(12) << s[i] << std::setw(12) << c[i]
                << std::setprecision(4) << std::setw(10) << p[i] << (rej[i] ? "   reject" : "   keep") << "\n";
    std::cout << "selected rank " << r.selected_rank << "\n\n";
  }
}
```

Output

```text
trace statistic
 H0: rank <=  eigenvalue   statistic 5% critical   p-value
           0      0.2226      108.34       29.66    0.0000   reject
           1      0.0186        7.85       15.46    0.4806   keep
           2      0.0010        0.38        3.85    0.5400   keep
selected rank 1

maximum-eigenvalue statistic
 H0: rank <=  eigenvalue   statistic 5% critical   p-value
           0      0.2226      100.49       21.06    0.0000   reject
           1      0.0186        7.47       14.23    0.4336   keep
           2      0.0010        0.38        3.85    0.5400   keep
selected rank 1
```

Read down the rows. "Rank ≤ 0" is rejected decisively by both statistics: there is at least one relation. "Rank ≤ 1" is not rejected, so the search stops there and the selected rank is 1: two of the three series are tied together and the third is not, which is exactly how the data were built. The eigenvalues are the squared canonical correlations between the changes and the lagged levels: a large one marks a direction in which the lagged level predicts the change strongly (a long-run relation), and small ones mark the common trends. The statistic is the sum of the logs of the *remaining* eigenvalues for the trace test and a single one for the maximum-eigenvalue test, so the two usually agree; when they do not, the trace statistic is generally the more robust. Note the stopping rule: the sequence ends at the first non-rejection, and a later rejection is ignored.

## How reliable is the selected rank?

A test of a rank is itself an estimate, and it makes mistakes. The program repeats the test on 25 independent samples of three different systems: no relation (three independent walks, rank 0), one relation (rank 1, as above) and two relations (`x` a walk, `y = x + u`, `z = x + v`, rank 2), at two sample sizes, and tabulates the selected rank.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

// kind 0: three independent walks; kind 1: y tied to x; kind 2: y and z both tied to x.
Series system(int kind, int n, std::uint64_t seed, const pluss::Config& cfg) {
  ts::ArimaFit white;
  white.sigma = 0.5;
  ts::ArimaFit ar;
  ar.ar = {0.6};
  ar.sigma = 0.5;
  const auto x = white.simulate(n, cfg, seed).cumsum(0).host<double>();
  const auto u = ar.simulate(n, cfg, seed + 1).host<double>();
  const auto v = ar.simulate(n, cfg, seed + 3).host<double>();
  const auto w = white.simulate(n, cfg, seed + 2).cumsum(0).host<double>();
  const auto z0 = white.simulate(n, cfg, seed + 4).cumsum(0).host<double>();
  std::vector<double> y(3 * n);
  for (int t = 0; t < n; ++t) {
    y[3 * t] = x[t];
    y[3 * t + 1] = kind == 0 ? w[t] : x[t] + u[t];
    y[3 * t + 2] = kind == 0 ? z0[t] : kind == 1 ? w[t] : x[t] + v[t];
  }
  return pluss::make_series(y, {n, 3}).to(pluss::ScalarType::Float64);
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const char* names[] = {"rank 0", "rank 1", "rank 2"};
  const int reps = 25;
  std::cout << std::left << std::setw(10) << "truth" << std::right << std::setw(6) << "n" << "   selected rank: 0    1    2    3\n";
  for (int kind = 0; kind <= 2; ++kind) {
    for (int n : {100, 400}) {
      int count[4] = {0, 0, 0, 0};
      for (int r = 0; r < reps; ++r) {
        ts::JohansenOptions o;
        o.lag_order = 1;
        ++count[ts::johansen_test(system(kind, n, 1000 + 10 * r, f64), o, f64).selected_rank];
      }
      std::cout << std::left << std::setw(10) << names[kind] << std::right << std::setw(6) << n << "                 ";
      for (int k = 0; k < 4; ++k) std::cout << std::setw(5) << count[k];
      std::cout << "\n";
    }
  }
}
```

Output

```text
truth          n   selected rank: 0    1    2    3
rank 0       100                    23    1    1    0
rank 0       400                    23    1    1    0
rank 1       100                     5   16    4    0
rank 1       400                     0   22    3    0
rank 2       100                     0    0   25    0
rank 2       400                     0    0   25    0
```

The right rank is the most common answer in every row, but the quality depends on the case. With no relation, 23 of 25 samples correctly select rank 0, and the two errors find a relation that is not there (the test has a 5% size, so about one in twenty is expected). With one relation and 100 observations only 16 of 25 get it right: five miss the relation (a weak long-run effect is hard to see in a short sample) and four report an extra one; with 400 observations the right rank is found 22 times and the errors are all in the upward direction. Two relations are the easiest here, because both stationary combinations are strong, and all 25 samples get them in both sizes. (The three systems use the same seeds across sample sizes, so the shorter sample is a prefix of the longer one.) The practical consequences are to use as many observations as you can, to use the economics (does a relation make sense?) alongside the test, and to treat a borderline result as a reason to look at the fitted model rather than a verdict.

## Decisions and pitfalls

- **Pass levels, not differences.** The test works on the levels of the series; differencing them first removes exactly the information it uses.
- **State the deterministic terms.** An unrestricted constant (the default) is right when the series drift; add a trend only if the levels trend deterministically; use neither only when the series are centred on zero. Wrong terms shift the critical values.
- **Set the lag order.** `lag_order` is the order `p` of the VAR in levels; `p − 1` lagged differences are held fixed. Choose it first, with the criteria of [Fitting a VAR](fitting_a_var_and_choosing_the_lag_order.md).
- **Read the sequence, not one row.** The rank is the first non-rejected hypothesis in the sequence.
- **Check every series has a unit root first.** If a series is already stationary it shows up as an extra relation; test each series with ADF and KPSS ([Chapter 1](../ts_01_describing_a_series/white_noise_unit_roots_stationarity.md)).

Where next: [The error-correction model](the_error_correction_model.md) fits the model once the rank is known.
