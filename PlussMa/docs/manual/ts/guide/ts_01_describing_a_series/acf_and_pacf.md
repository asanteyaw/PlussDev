<!-- generated from acf_and_pacf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# ACF and PACF

[Back to Describing a Series](../ts_01_describing_a_series.md)

The autocorrelation function (ACF) measures how strongly a series is related to its own past. The autocorrelation at lag `k` is the correlation between `x_t` and `x_{t−k}`; plotted or printed against `k` it is a fingerprint of the process. The partial autocorrelation function (PACF) asks a sharper question: what is the relation at lag `k` *after removing* the effect of the lags in between? The two functions look different for different processes, and the differences are how the orders of an ARIMA model are read off a series before any model is fitted:

| Process | ACF | PACF |
|---|---|---|
| AR(p) | decays gradually (geometrically or in waves) | cuts off after lag p |
| MA(q) | cuts off after lag q | decays gradually |
| ARMA(p, q) | decays gradually | decays gradually |
| random walk / trend | decays very slowly, stays near 1 | one large value at lag 1 |
| seasonal with period s | peaks at lags s, 2s, … | spike at lag s |

`acf(x, options)` and `pacf(x, options)` return a `Series` with one value per lag from 0 (the autocorrelation at lag 0 is always 1) up to `max_lag`. The options are `max_lag` (default 20), `norm` (`Biased` divides the autocovariance by `n`, `Unbiased` by `n − k`) and `demean` (subtract the sample mean first, on by default).

## Reading an AR(1)

The first program draws 200 observations from an AR(1) with coefficient 0.7 and prints the sample ACF under both normalisations beside the theoretical value `0.7^k`, then the PACF and the 95% band for white noise of this length, `±1.96/√n`. A sample autocorrelation is itself random, so the sample values scatter around the theory; the band tells you how much scatter to expect from a series with no dependence at all.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::ArimaFit truth;
  truth.ar = {0.7};
  const Series x = truth.simulate(200, f64, /*seed=*/4);

  ts::AcfOptions a;
  a.max_lag = 6;
  out.writeln("acf biased", ts::acf(x, a).host<double>());
  a.norm = ts::AcfNorm::Unbiased;
  out.writeln("acf unbiased", ts::acf(x, a).host<double>());
  std::cout << std::left << std::setw(14) << "0.7^k" << std::right;
  for (int k = 0; k <= 6; ++k) std::cout << std::setw(8) << std::pow(0.7, k);
  std::cout << "\n";

  ts::PacfOptions p;
  p.max_lag = 6;
  out.writeln("pacf", ts::pacf(x, p).host<double>());
  std::cout << "\n95% band for white noise, n = 200: +-" << 1.96 / std::sqrt(200.0) << "\n";
}
```

Output

```text
acf biased [1.000, 0.762, 0.584, 0.402, 0.232, 0.062, -0.036]
acf unbiased [1.000, 0.765, 0.589, 0.408, 0.237, 0.063, -0.037]
0.7^k                1     0.7    0.49   0.343  0.2401 0.168070.117649
pacf [1.000, 0.762, 0.008, -0.107, -0.102, -0.136, 0.020]

95% band for white noise, n = 200: +-0.138593
```

The sample ACF starts at 0.76 (theory 0.70) and decays geometrically, but it does not track `0.7^k` exactly: by lag 5 it has fallen to 0.06 where the theory says 0.17. That is sampling error, and it grows with the lag because fewer pairs of observations are available; with 200 observations each autocorrelation carries an error of several hundredths. The PACF shows the structure more cleanly: one large value at lag 1 and then values inside the ±0.139 band (the largest, −0.136 at lag 5, is just inside it). One significant partial autocorrelation followed by nothing is the signature of an AR(1). The unbiased normalisation inflates the high lags slightly because it divides by the shrinking `n − k`; the biased one is the usual choice because it always produces a valid (positive-definite) autocorrelation sequence, which matters if you use the values to estimate a model.

## Telling AR from MA

The cut-off rules are easiest to see by putting two processes next to each other. The program below simulates an AR(2) and an MA(2) of the same length, prints both functions for each, together with the half-width of the white-noise band: any value beyond it, at lags 1 and up, is significant at the 5% level.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const int n = 400;
  ts::ArimaFit ar2;
  ar2.ar = {0.5, 0.3};
  ts::ArimaFit ma2;
  ma2.ma = {0.7, 0.4};
  const Series xa = ar2.simulate(n, f64, /*seed=*/12);
  const Series xm = ma2.simulate(n, f64, /*seed=*/13);

  ts::AcfOptions a;
  a.max_lag = 8;
  ts::PacfOptions p;
  p.max_lag = 8;
  out.writeln("white-noise band, +-", 1.96 / std::sqrt(static_cast<double>(n)), PrintOptions{}.fixed(3));
  out.writeln("AR(2)  acf ", ts::acf(xa, a).host<double>());
  out.writeln("AR(2)  pacf", ts::pacf(xa, p).host<double>());
  out.writeln("MA(2)  acf ", ts::acf(xm, a).host<double>());
  out.writeln("MA(2)  pacf", ts::pacf(xm, p).host<double>());
}
```

Output

```text
white-noise band, +- 0.098
AR(2)  acf  [1.00, 0.61, 0.52, 0.40, 0.33, 0.23, 0.18, 0.20, 0.14]
AR(2)  pacf [1.00, 0.61, 0.24, 0.01, 0.03, -0.05, -0.01, 0.12, -0.04]
MA(2)  acf  [1.00, 0.57, 0.15, -0.09, -0.05, -0.03, -0.07, -0.09, -0.08]
MA(2)  pacf [1.00, 0.57, -0.26, -0.08, 0.14, -0.11, -0.07, 0.01, -0.03]
```

For the AR(2) the PACF is significant at lags 1 and 2 and then stops (the lone value beyond the band at lag 7, 0.12 against 0.098, is the chance exceedance a 95% band lets through), while the ACF stays significant across all eight lags. For the MA(2) the ACF is significant at lags 1 and 2 and then stops cleanly, while the PACF does not: it is large at lags 1 and 2 and keeps producing marginal values (lags 4 and 5 here). That is the rule in the table: **the function that cuts off names the order** (PACF for an autoregression, ACF for a moving average). Expect occasional values beyond the band after the cut-off, since about one lag in twenty exceeds a 95% band by chance; a single marginal value at a high lag is not evidence of structure.

## Trends and seasonality

Two common shapes show up in the ACF before they show up anywhere else. A series with a stochastic trend has autocorrelations that stay close to 1 and fall very slowly; a seasonal series has peaks at the seasonal lag and its multiples. Both are a message that the series has not been made stationary yet: difference it (or seasonally difference it) and look again.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(2));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  ts::ArimaFit noise;
  const Series walk = noise.simulate(300, f64, /*seed=*/2).cumsum(0);

  ts::ArimaFit seasonal;
  seasonal.sar = {0.8};
  seasonal.seasonal_period = 4;
  const Series monthly = seasonal.simulate(300, f64, /*seed=*/6);

  ts::AcfOptions a;
  a.max_lag = 12;
  std::cout << "lag                         ";
  for (int k = 0; k <= 12; ++k) std::cout << std::setw(6) << k;
  std::cout << "\n";
  out.writeln("random walk", ts::acf(walk, a).host<double>());
  out.writeln("seasonal AR, period 4", ts::acf(monthly, a).host<double>());
}
```

Output

```text
lag                              0     1     2     3     4     5     6     7     8     9    10    11    12
random walk [1.00, 0.95, 0.90, 0.85, 0.82, 0.78, 0.74, 0.70, 0.67, 0.64, 0.62, 0.60, 0.58]
seasonal AR, period 4 [1.00, -0.06, -0.22, -0.04, 0.75, -0.08, -0.23, 0.02, 0.52, -0.12, -0.22, 0.09, 0.36]
```

The walk's autocorrelation starts at 0.95 and loses only a few hundredths per lag, still 0.58 at lag 12: no cut-off and almost no decay, which is how a unit root looks. The seasonal series peaks at lags 4, 8 and 12 (0.75, 0.52, 0.36) and is near zero or negative in between, the repeating pattern of a period-4 seasonal with its shrinking height. In both cases the next step is a transform from [the previous page](the_transforms.md) or a test from [White noise, unit roots, stationarity](white_noise_unit_roots_stationarity.md).

## Decisions and pitfalls

- **The band is a guide, not a test.** It is the 95% region for a white-noise series, so one value outside it is unremarkable; look for patterns (a run of significant lags, a clear cut-off).
- **Do not interpret a PACF value you cannot afford.** Beyond lag `n/4` or so the estimates are poor; keep `max_lag` modest.
- **Leave `demean` on** for raw data. Turn it off only when the mean is known to be zero (for example model residuals).
- **Prefer the biased normalisation** unless you have a reason; it is the one that guarantees a valid autocorrelation sequence.

Where next: [White noise, unit roots, stationarity](white_noise_unit_roots_stationarity.md) turns the visual impression into formal tests.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_transforms.html">
    <span class="label">Previous</span>
    <span class="title">The transforms</span>
  </a>
  <a class="next-link" href="white_noise_unit_roots_stationarity.html">
    <span class="label">Next</span>
    <span class="title">White noise, unit roots, stationarity</span>
  </a>
</div>
<!-- apiref:nav:end -->
