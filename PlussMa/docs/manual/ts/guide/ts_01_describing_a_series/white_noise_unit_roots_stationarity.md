<!-- generated from white_noise_unit_roots_stationarity.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# White noise, unit roots, stationarity

[Back to Describing a Series](../ts_01_describing_a_series.md)

The ACF and PACF give an impression; a test gives a decision with a stated error rate. Three questions come up before any model is fitted, and each has its own test:

| Question | Test | Null hypothesis | Rejecting it means |
|---|---|---|---|
| Is the series just noise, with no dependence on its past? | `ljung_box` | all autocorrelations up to the lag are zero | there is dependence to model |
| Does the series have a unit root (a stochastic trend)? | `adf_test` | a unit root | the series is stationary |
| Is the series stationary? | `kpss_test` | stationary (around a level or a trend) | a unit root, or a trend that was not modelled |

Each returns a statistic, a p-value, the critical value at the chosen `alpha` and a boolean `reject`. ADF and KPSS have *opposite* nulls, which is the reason to use them together: one test failing to reject is weak evidence, two tests agreeing is not.

## Is it white noise?

The Ljung–Box statistic combines the squared autocorrelations up to a chosen lag into one number that is compared with a χ² distribution. `LjungBoxOptions::lags` takes several lags at once, so one call produces the statistic and p-value at each. The program compares independent noise with an AR(1) and with the *residuals* of an AR(1) fit, where `model_df` says how many ARMA coefficients were estimated: the degrees of freedom are reduced by that number, so a correct model passes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  ts::ArimaFit white;
  const Series noise = white.simulate(300, f64, /*seed=*/1);

  ts::ArimaFit truth;
  truth.ar = {0.6};
  const Series x = truth.simulate(300, f64, /*seed=*/1);

  ts::ArimaOptions o;
  o.ar_order = 1;
  const Series resid = ts::Arima::fit_mle(x, o, f64).infer(x, f64);

  const std::vector<const char*> labels = {"white noise", "AR(1) series", "AR(1) residuals, df 0", "AR(1) residuals, df 1"};
  const std::vector<Series> series = {noise, x, resid, resid};
  const std::vector<std::int64_t> model_df = {0, 0, 0, 1};

  out.writeln("Ljung-Box at lags 5, 10, 20");
  for (std::size_t i = 0; i < series.size(); ++i) {
    ts::LjungBoxOptions lb;
    lb.lags = {5, 10, 20};
    lb.model_df = model_df[i];
    const auto r = ts::ljung_box(series[i], lb);
    out.writeln(labels[i], "Q", r.stat.host<double>(), "p", r.pvalue.host<double>(), "df", r.dof.to(pluss::ScalarType::Int64).host<std::int64_t>());
  }
}
```

Output

```text
Ljung-Box at lags 5, 10, 20
white noise Q [4.9320, 7.2597, 12.8936] p [0.4242, 0.7007, 0.8819] df [5, 10, 20]
AR(1) series Q [153.7931, 158.7249, 195.2982] p [0.0000, 0.0000, 0.0000] df [5, 10, 20]
AR(1) residuals, df 0 Q [3.2839, 5.5327, 12.4990] p [0.6563, 0.8529, 0.8978] df [5, 10, 20]
AR(1) residuals, df 1 Q [3.2839, 5.5327, 12.4990] p [0.5115, 0.7856, 0.8632] df [4, 9, 19]
```

Read it as three cases. The AR(1) series is far from white noise: the p-values are essentially zero at every lag. Independent noise passes at every lag. The residuals of the fitted AR(1) pass too, which is the point of fitting a model, and the only difference between the last two lines is the correct degrees of freedom: the model absorbed one coefficient, so the χ² reference has one degree less, and ignoring that makes the test too lenient. Always set `model_df` to the number of ARMA coefficients when testing residuals.

## Does it have a unit root?

A series with a unit root, such as a random walk, wanders without returning to a mean, and shocks to it never die out. Modelling it as if it were stationary gives coefficients near 1 and forecasts that mean-revert when they should not, so the question has to be answered before a model is fitted. The Augmented Dickey–Fuller test regresses the changes of the series on its lagged level and tests whether that level matters. `AdfOptions::model` chooses the deterministic part of the regression:

- `AR`: no constant (the series is centred around zero),
- `ARD`: a constant (the default: stationary around a nonzero level),
- `TS`: a constant and a linear trend (stationary around a trend).

`lags` adds lagged changes to absorb serial correlation in the errors. The p-values and critical values come from response surfaces that depend on the sample size, not from the asymptotic table.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  ts::ArimaFit st;
  st.ar = {0.7};
  const Series stationary = st.simulate(250, f64, /*seed=*/9);
  ts::ArimaFit en;
  const Series walk = en.simulate(250, f64, /*seed=*/9).cumsum(0);

  const ts::AdfModel models[] = {ts::AdfModel::AR, ts::AdfModel::ARD, ts::AdfModel::TS};
  const char* names[] = {"AR (none)", "ARD (const)", "TS (trend)"};
  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::left << std::setw(14) << "model" << std::right << std::setw(26) << "stationary AR(1)" << std::setw(28) << "random walk" << "\n";
  std::cout << std::left << std::setw(14) << "" << std::right << std::setw(10) << "stat" << std::setw(9) << "crit5%" << std::setw(7) << "p"
            << std::setw(10) << "stat" << std::setw(9) << "crit5%" << std::setw(7) << "p" << "\n";
  for (int i = 0; i < 3; ++i) {
    ts::AdfOptions o;
    o.model = models[i];
    const auto a = ts::adf_test(stationary, o);
    const auto b = ts::adf_test(walk, o);
    std::cout << std::left << std::setw(14) << names[i] << std::right << std::setw(10) << a.stat << std::setw(9) << a.critical << std::setw(7) << a.pvalue
              << std::setw(10) << b.stat << std::setw(9) << b.critical << std::setw(7) << b.pvalue << "\n";
  }
}
```

Output

```text
model                   stationary AR(1)                 random walk
                    stat   crit5%      p      stat   crit5%      p
AR (none)         -7.387   -1.942  0.000     0.517   -1.942  0.829
ARD (const)       -7.605   -2.873  0.000    -1.132   -2.873  0.702
TS (trend)        -7.589   -3.428  0.000    -3.602   -3.428  0.030
```

The test rejects (statistic below the critical value, small p) for the stationary series in all three specifications. For the random walk it fails to reject with no constant (p = 0.83) and with a constant (p = 0.70), the correct outcome. The trend specification is more delicate: its statistic, −3.60, falls just past the 5% critical value of −3.43 (p = 0.03), so it rejects a unit root that is really there. At a 5% level a correct test makes this mistake about one time in twenty, and a trend term makes the test less powerful and the decision shakier, which is exactly why a single ADF result should not be trusted on its own. Because the null is a unit root, *failing to reject* is the finding that matters for a series that looks like it trends, and it is the less reliable one: with a short sample or a root close to 1 the test has little power. That weakness is why a second test with the opposite null is worth running.

## Is it stationary?

The KPSS test turns the question around: its null is that the series is stationary (around a level, or around a trend when `trend` is true), and it rejects when the accumulated deviations from that level are too large. Together with ADF it gives a decision table:

| ADF | KPSS | Reading |
|---|---|---|
| rejects | does not reject | stationary |
| does not reject | rejects | unit root: difference the series |
| rejects | rejects | the evidence conflicts (often a structural break or a near unit root) |
| does not reject | does not reject | the sample is not informative enough |

The program runs both tests over four series, one of each kind, and prints the decision each pair implies. KPSS needs `lags` above zero when the series is autocorrelated, because its long-run variance estimate must account for the dependence; a short rule-of-thumb value is used here.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;
using namespace pluss::tils;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));

  ts::ArimaFit ar;
  ar.ar = {0.5};
  ts::ArimaFit noise;
  const Series stationary = ar.simulate(300, f64, /*seed=*/21);
  const Series walk = noise.simulate(300, f64, /*seed=*/21).cumsum(0);
  // A deterministic trend plus stationary noise: trend-stationary, not a unit root.
  const Series trending = pluss::series::linspace(0.0, 0.05 * 299.0, 300, f64) + stationary;

  const std::vector<const char*> labels = {"level model: stationary AR(1)", "level model: random walk", "level model: trend + AR(1) noise", "trend model: trend + AR(1) noise", "trend model: random walk"};
  const std::vector<Series> series = {stationary, walk, trending, trending, walk};
  const std::vector<bool> with_trend = {false, false, false, true, true};

  for (std::size_t i = 0; i < series.size(); ++i) {
    ts::AdfOptions ao;
    ao.model = with_trend[i] ? ts::AdfModel::TS : ts::AdfModel::ARD;
    ao.lags = 2;
    ts::KpssOptions ko;
    ko.trend = with_trend[i];
    ko.lags = 8;
    const auto a = ts::adf_test(series[i], ao);
    const auto k = ts::kpss_test(series[i], ko);
    const std::string reading = a.reject && !k.reject ? "stationary" : !a.reject && k.reject ? "unit root: difference it" : a.reject && k.reject ? "conflict" : "inconclusive";
    out.writeln(labels[i], "ADF p", a.pvalue, a.reject ? "reject" : "keep", "KPSS p", k.pvalue, k.reject ? "reject" : "keep", "->", reading);
  }
}
```

Output

```text
level model: stationary AR(1) ADF p 0.000 reject KPSS p 0.100 keep -> stationary
level model: random walk ADF p 0.377 keep KPSS p 0.010 reject -> unit root: difference it
level model: trend + AR(1) noise ADF p 0.486 keep KPSS p 0.010 reject -> unit root: difference it
trend model: trend + AR(1) noise ADF p 0.000 reject KPSS p 0.100 keep -> stationary
trend model: random walk ADF p 0.302 keep KPSS p 0.010 reject -> unit root: difference it
```

The two tests agree on the stationary series (ADF rejects, KPSS keeps; the KPSS p-value of 0.10 is the largest the tables provide) and on the random walk (ADF keeps, KPSS rejects, p = 0.01 being the smallest the tables provide). The third series is the instructive one: it has a deterministic trend but is stationary around it, and the verdict depends on the specification. With the level model the trend makes both tests call it a unit-root series, and with the trend term in both tests it reads as stationary. Choosing `TS` / `trend = true` is therefore a modelling statement: use it when the series has a drift you believe is deterministic, and difference instead when you believe the trend is stochastic.

## Decisions and pitfalls

- **Use both tests.** A single unit-root test with a short sample proves little; two tests with opposite nulls that agree prove more.
- **Match the specification to the series.** A trending series tested with the level model will fail to reject for the wrong reason.
- **Set `lags` when the errors are autocorrelated.** For ADF, add lagged changes until the residual autocorrelation is gone; for KPSS, use a positive bandwidth.
- **Mind `model_df` for residuals.** Without it a well-fitted model looks slightly better than it is.
- **One rejection is not the whole story.** A series can have a unit root *and* a structural break, and the tests will disagree; look at the plot.

Where next: [Volatility clustering](volatility_clustering.md) tests the other half of "is it noise": whether the *size* of the movements depends on the past.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="acf_and_pacf.html">
    <span class="label">Previous</span>
    <span class="title">ACF and PACF</span>
  </a>
  <a class="next-link" href="volatility_clustering.html">
    <span class="label">Next</span>
    <span class="title">Volatility clustering</span>
  </a>
</div>
<!-- apiref:nav:end -->
