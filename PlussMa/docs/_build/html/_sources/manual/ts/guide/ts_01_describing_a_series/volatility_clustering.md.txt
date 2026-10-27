<!-- generated from volatility_clustering.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Volatility clustering

[Back to Describing a Series](../ts_01_describing_a_series.md)

A series can be unpredictable in its *level* and still be predictable in its *size*. Financial returns are the classic case: the direction of tomorrow's return is close to a coin toss, but a large move today makes a large move tomorrow more likely, and calm periods follow calm periods. This is **volatility clustering**, and it means the variance of the series is not constant but depends on the recent past. A model that assumes a constant variance (an ordinary ARIMA, for instance) understates risk in turbulent stretches and overstates it in quiet ones.

The ACF of the series itself will not show it, because the *signs* of the movements are unrelated. What shows it is the dependence among the *squares* (or absolute values): if big values cluster, the squares are autocorrelated. The ARCH-LM test makes this precise: it regresses the squared, mean-removed series on its own `lags` past values and tests whether those coefficients are all zero. Its null hypothesis is **no volatility clustering**, and a small p-value rejects it.

The three programs below use two simulated series of the same length: independent noise with constant variance, and a GARCH(1,1) series whose variance follows `σ²_t = ω + α e²_{t−1} + β σ²_{t−1}`.

## The test

`arch_lm(x, {lags, demean, alpha})` returns the statistic (`stat`), p-value, the critical value, the decision (`reject`) and the degrees of freedom (`dof`, equal to `lags`). With `demean` on (the default) the sample mean is removed before squaring, which is right for raw returns; set it to false for the residuals of a mean model, whose mean is zero by construction.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  ts::ArimaFit white;
  white.sigma = 1.0;
  const Series steady = white.simulate(500, f64, /*seed=*/5);

  ts::Garch11Fit g;
  g.omega = 0.2;
  g.alpha = 0.25;
  g.beta = 0.65;
  g.mean = 0.0;
  const Series clustered = g.simulate(500, f64, /*seed=*/5);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(12) << "series" << std::right << std::setw(7) << "lags" << std::setw(10) << "LM" << std::setw(12) << "p-value"
            << std::setw(12) << "critical" << "  decision\n";
  for (int lags : {1, 5, 10}) {
    ts::ArchLmOptions o;
    o.lags = lags;
    const auto a = ts::arch_lm(steady, o);
    const auto b = ts::arch_lm(clustered, o);
    std::cout << std::left << std::setw(12) << "steady" << std::right << std::setw(7) << lags << std::setw(10) << a.stat << std::setw(12) << a.pvalue
              << std::setw(12) << a.critical << (a.reject ? "  reject\n" : "  keep\n");
    std::cout << std::left << std::setw(12) << "clustered" << std::right << std::setw(7) << lags << std::setw(10) << b.stat << std::setw(12) << b.pvalue
              << std::setw(12) << b.critical << (b.reject ? "  reject\n" : "  keep\n");
  }
}
```

Output

```text
series         lags        LM     p-value    critical  decision
steady            1    3.8840      0.0487      3.8415  reject
clustered         1    7.4295      0.0064      3.8415  reject
steady            5    8.6597      0.1234     11.0705  keep
clustered         5   41.1154      0.0000     11.0705  reject
steady           10    9.9321      0.4465     18.3070  keep
clustered        10   42.8499      0.0000     18.3070  reject
```

The clustered series rejects at every lag, and at five and ten lags the statistic (41 and 43) is several times the critical value, with p-values below 10⁻⁴. The steady series keeps the null at five and ten lags, but at one lag it *just* rejects (p = 0.049). That is not a bug: at a 5% level a correct test rejects true nulls about one time in twenty, and a borderline p-value at a single lag should not be acted on. Compare the two series across lags rather than at one: a decisive rejection that persists (clustered) is evidence, an isolated marginal one (steady, lag 1) is not. A model for the variance (see the [volatility models](../ts_03_volatility_models.md) chapter) is justified for the second series and unnecessary for the first.

## Where the evidence comes from

The test statistic hides *why* it rejects. Compare the autocorrelations of the values themselves with those of their squares. For the clustered series the values are close to uncorrelated (so a mean model has nothing to find) while the squares are clearly autocorrelated and decay slowly; for the steady series neither is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <cmath>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  ts::ArimaFit white;
  white.sigma = 1.0;
  const Series steady = white.simulate(500, f64, /*seed=*/5);
  ts::Garch11Fit g;
  g.omega = 0.2;
  g.alpha = 0.25;
  g.beta = 0.65;
  const Series clustered = g.simulate(500, f64, /*seed=*/5);

  ts::AcfOptions a;
  a.max_lag = 6;
  std::cout << "autocorrelation at lags 0..6\n";
  out.writeln("steady, values", ts::acf(steady, a).host<double>());
  out.writeln("steady, squares", ts::acf(steady * steady, a).host<double>());
  out.writeln("clustered, values", ts::acf(clustered, a).host<double>());
  out.writeln("clustered, squares", ts::acf(clustered * clustered, a).host<double>());
  std::cout << "\n95% band: +-" << 1.96 / std::sqrt(500.0) << "\n";
}
```

Output

```text
autocorrelation at lags 0..6
steady, values [1.000, -0.033, -0.101, -0.019, -0.047, 0.029, 0.015]
steady, squares [1.000, -0.090, -0.029, 0.038, 0.045, 0.052, -0.014]
clustered, values [1.000, -0.062, -0.118, 0.018, -0.061, 0.066, 0.009]
clustered, squares [1.000, 0.118, 0.076, 0.201, 0.181, 0.144, 0.089]

95% band: +-0.0876539
```

For the steady series nothing exceeds the ±0.088 band by much, in the values or the squares. For the clustered series the values stay small (one lag marginally outside the band) while the squares are positive at every lag and exceed the band at five of the six lags. This is the diagnostic worth remembering: **the values are barely predictable, their squares are.** If the squares of a series (or of a model's residuals) are autocorrelated, an assumption of constant variance is wrong regardless of how well the mean is modelled.

## Seeing it in the data

A rolling standard deviation makes the clustering visible without any test. For constant-variance noise a 25-observation rolling standard deviation wanders only because a short window is a noisy estimate; for the clustered series it also reflects genuine calm and turbulent stretches, so it ranges more widely.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  ts::ArimaFit white;
  white.sigma = 1.0;
  ts::Garch11Fit g;
  g.omega = 0.2;
  g.alpha = 0.25;
  g.beta = 0.65;
  const std::vector<std::pair<const char*, Series>> cases = {{"steady", white.simulate(500, f64, /*seed=*/5)}, {"clustered", g.simulate(500, f64, /*seed=*/5)}};

  for (const auto& [label, x] : cases) {
    const Series sd = ts::rolling_std(x, 25, 25);
    const Series valid = sd.masked_select(sd == sd);  // drop the NaN start-up values
    const double lo = valid.min().item<double>(), hi = valid.max().item<double>();
    out.writeln(label, "rolling std: min", lo, "max", hi, "ratio", hi / lo);
  }
}
```

Output

```text
steady rolling std: min 0.612 max 1.307 ratio 2.133
clustered rolling std: min 0.548 max 2.015 ratio 3.679
```

The clustered series swings between a rolling standard deviation of 0.55 and 2.0, a ratio of 3.7, while the steady one stays between 0.61 and 1.31 (ratio 2.1). Note that the steady series also moves: a 25-observation standard deviation is itself a noisy estimate, so a ratio above 1 is normal even for constant variance. The rolling picture is a first look that shows *where* the turbulent stretches are, and the LM test is the decision.

## Decisions and pitfalls

- **Test the residuals, not just the returns.** After fitting a mean model, run `arch_lm` on its residuals with `demean = false`; ARCH effects left in the residuals invalidate the standard errors and the forecast intervals of the mean model.
- **Pick `lags` to match the memory you expect.** One lag detects short clustering; a few more guard against slower decay. A handful of lags is usually enough.
- **Rejecting is a reason to model the variance, not a reason to discard the series.** Chapter 3 fits GARCH, GJR and EGARCH models.
- **Outliers can mimic clustering.** One enormous observation inflates the squares for the whole sample; look at the series before concluding.

Where next: [Time Series 2: ARIMA Models](../ts_02_arima_models.md) fits a model to the mean of a series once these checks are done.
