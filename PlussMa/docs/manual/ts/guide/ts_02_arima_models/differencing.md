<!-- generated from differencing.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Differencing

[Back to ARIMA Models](../ts_02_arima_models.md)

The "I" in ARIMA stands for *integrated*: a series is integrated of order `d` if it must be differenced `d` times to become stationary. Differencing replaces `x_t` by `x_t − x_{t−1}`, the change since the last observation. A random walk wanders with no level to return to, but its changes are plain noise; a series with a trend in its level has changes that fluctuate around a constant. Fitting an ARMA model to the differences and then adding the changes back up (which `forecast` does for you) is how ARIMA handles a series that is not stationary.

Getting `d` right matters in both directions. Too few differences leave a unit root in the data: the autoregressive coefficient is pinned near 1, the estimates are unreliable and the forecasts have the wrong shape. Too many differences destroy information and inject a moving-average component with a coefficient near −1, making the series noisier than it was. This page shows how to choose `d`, what each error looks like, and how drift enters.

The options are `diff_order` (the ordinary difference) in `ArimaOptions`, and `seasonal_diff_order` (the difference at the seasonal lag), covered in [Seasonal models](seasonal_models.md).

## Choosing `d` from the data

Two pieces of evidence agree when the choice is right: the unit-root test stops finding a unit root after differencing, and the ACF stops decaying slowly. The program takes a random walk and its first difference and prints both. `diff` returns a series of the same length with a leading `NaN`, which is dropped before the test.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

Series drop_first(const Series& s, const pluss::Config& cfg) {
  auto v = s.items<double>();
  v.erase(v.begin());
  return pluss::make_series(v, cfg);
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::ArimaFit noise;
  noise.sigma = 1.0;
  const Series walk = noise.simulate(300, f64, /*seed=*/15).cumsum(0);
  const Series changes = drop_first(ts::diff(walk, 1), f64);

  ts::AdfOptions adf;
  adf.lags = 2;
  ts::KpssOptions kpss;
  kpss.trend = false;
  kpss.lags = 8;
  ts::AcfOptions a;
  a.max_lag = 4;

  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::left << std::setw(14) << "" << std::right << std::setw(10) << "ADF p" << std::setw(10) << "KPSS p" << "   acf at lags 1..4\n";
  for (int d = 0; d <= 1; ++d) {
    const Series& x = d == 0 ? walk : changes;
    const auto ac = ts::acf(x, a).host<double>();
    std::cout << std::left << std::setw(14) << (d == 0 ? "level (d = 0)" : "diff  (d = 1)") << std::right << std::setw(10) << ts::adf_test(x, adf).pvalue << std::setw(10)
              << ts::kpss_test(x, kpss).pvalue << "  ";
    for (std::size_t k = 1; k < ac.size(); ++k) std::cout << std::setw(7) << ac[k];
    std::cout << "\n";
  }
}
```

Output

```text
                   ADF p    KPSS p   acf at lags 1..4
level (d = 0)      0.478     0.010    0.976  0.952  0.931  0.908
diff  (d = 1)      0.000     0.100    0.001 -0.103  0.027 -0.057
```

In levels, ADF cannot reject a unit root, KPSS rejects stationarity and the autocorrelations stay close to 1; after one difference ADF rejects, KPSS keeps stationarity and the autocorrelations vanish. That is the pattern that justifies `diff_order = 1`. If the differenced series still failed the tests you would difference again, but series needing `d = 2` are rare in practice and a second difference should be a considered choice.

## Over-differencing

Differencing a series that is already stationary does damage that is easy to miss, because the model still fits. The program compares the variance of a series after 0, 1 and 2 differences: it falls sharply when differencing is needed (a random walk) and rises when it is not (a mildly autocorrelated AR(1) with coefficient 0.3). Then it takes independent noise, which needs no differencing at all, and fits it with an ARIMA(0,1,1), to see what the extra difference turns into.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  ts::ArimaFit stationary;
  stationary.ar = {0.3};
  const Series x = stationary.simulate(400, f64, /*seed=*/18);
  ts::ArimaFit white;  // independent noise: nothing to difference at all
  const Series w = white.simulate(400, f64, /*seed=*/18);
  ts::ArimaFit noise;
  const Series walk = noise.simulate(400, f64, /*seed=*/18).cumsum(0);

  out.writeln("variance after 0, 1 and 2 differences");
  const std::vector<std::pair<const char*, Series>> cases = {{"stationary AR(1)", x}, {"random walk", walk}};
  for (const auto& [label, s] : cases) {
    std::vector<double> variance;
    Series z = s;
    for (int d = 0; d <= 2; ++d) {
      variance.push_back(z.var().item<double>());
      z = ts::diff(z, 1).slice(0, 1, z.numel());  // difference once more, dropping the undefined first value
    }
    out.writeln(label, variance);
  }

  ts::ArimaOptions o;
  o.ar_order = 0;
  o.diff_order = 1;
  o.ma_order = 1;
  const auto over = ts::Arima::fit_mle(w, o, f64);
  ts::ArimaOptions ok;
  ok.ar_order = 0;
  const auto right = ts::Arima::fit_mle(w, ok, f64);
  out.writeln("white noise fitted with ARIMA(0,0,0): sigma =", right.sigma);
  out.writeln("white noise fitted with ARIMA(0,1,1): ma =", over.ma[0], ", sigma =", over.sigma);
}
```

Output

```text
variance after 0, 1 and 2 differences
stationary AR(1) [1.080, 1.433, 3.656]
random walk [44.354, 0.961, 1.795]
white noise fitted with ARIMA(0,0,0): sigma = 0.978
white noise fitted with ARIMA(0,1,1): ma = -1.000 , sigma = 0.979
```

For the random walk the variance drops from 44 to 0.96 at the first difference and rises again to 1.8 at the second; for the AR(1) it is lowest with no differencing (1.08) and grows with every difference (1.43, then 3.66). The minimum-variance order is a rough guide, not a test: for a strongly autocorrelated stationary series (an AR(1) with a coefficient above 0.5) one difference can lower the variance even though none is needed, so confirm with a unit-root test. The last two lines show the footprint of over-differencing: the extra difference turns independent noise into a moving average whose coefficient is pinned at −1, the edge of invertibility (the difference cancels the unit root that the moving average put back). The fit is not hurt in sigma here, but the model is more complicated than it should be, has a parameter on a boundary, and its forecasts and standard errors are unreliable. If a fitted MA coefficient sits at or near −1 after differencing, suspect over-differencing before suspecting a genuinely complex model.

## Drift

Differencing removes a trend in the *level* but not a constant *change*: a series that rises by about 0.3 per period, on average, has changes with a mean of 0.3. With `demean` on (the default) the fit estimates that mean, and the integrated forecasts keep rising at that rate, so the level of the forecast continues along the trend. The program fits a random walk with drift and shows the estimate and the forecasts; it also shows what happens with a second difference, which would force the drift to zero.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  ts::ArimaFit changes;
  changes.intercept = 0.3;  // the average change per period
  changes.sigma = 1.0;
  const Series y = changes.simulate(300, f64, /*seed=*/9).cumsum(0);
  const double last = y.host<double>().back();

  ts::ArimaOptions o;
  o.ar_order = 0;
  o.diff_order = 1;
  const auto fit = ts::Arima::fit_mle(y, o, f64);
  const auto fc = fit.forecast(5, y, f64).host<double>();

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "estimated drift per period: " << fit.intercept << " (true 0.3000)\n";
  std::cout << "last value " << last << "; forecasts of the level:";
  for (double v : fc) std::cout << " " << v;
  std::cout << "\n";
  std::cout << "forecast increments:";
  double prev = last;
  for (double v : fc) { std::cout << " " << v - prev; prev = v; }
  std::cout << "\n";
}
```

Output

```text
estimated drift per period: 0.3023 (true 0.3000)
last value 90.6798; forecasts of the level: 90.9820 91.2843 91.5865 91.8888 92.1911
forecast increments: 0.3023 0.3023 0.3023 0.3023 0.3023
```

The forecasts of the level climb by the same amount at each step, the estimated drift (0.302 per period against a true 0.3). A random walk *without* drift would give a flat forecast at the last value. Whether to include drift is a substantive decision: a drift extrapolated over a long horizon dominates the forecast, so include it only when the trend is a feature of the process and not an accident of the sample.

## Decisions and pitfalls

- **Test, then difference.** Use the ADF and KPSS tests together; difference only if they indicate a unit root.
- **Prefer `d = 1`.** A second difference is rarely needed and amplifies noise.
- **Watch for an MA coefficient near −1.** It is the footprint of over-differencing.
- **Differencing is part of the model.** Fit with `diff_order` rather than differencing by hand, so that `forecast` returns levels and the likelihood is comparable across candidate models with the same `d`.
- **Likelihoods with different `d` are not comparable**, because they describe different series; compare models with the same `d` ([Choosing the orders](choosing_the_orders.md)).

Where next: [Chapter 3](../ts_03_volatility_models.md) turns from the mean of a series to its variance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="seasonal_models.html">
    <span class="label">Previous</span>
    <span class="title">Seasonal models</span>
  </a>
</div>
<!-- apiref:nav:end -->
