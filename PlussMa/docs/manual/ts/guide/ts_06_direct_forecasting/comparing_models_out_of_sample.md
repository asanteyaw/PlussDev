<!-- generated from comparing_models_out_of_sample.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Comparing models out of sample

[Back to Direct Forecasting and Model Comparison](../ts_06_direct_forecasting.md)

Which of two forecasting models is better? In-sample fit cannot answer this, because a bigger model always fits better. The answer has to come from forecasts of data the models did not train on, and it has two parts: *how much better* (a loss averaged over test data) and *is the difference real* (a test that allows for the randomness of a finite test sample). This page provides both.

**Cross-validated forecasts.** `DirectCVForecaster::fit(x, y, partition, options)` fits one direct forecaster per training window of a partition ([Partitions](partitions.md)). `cvpredict(x, y)` then returns, for every row that lies in some test set, the prediction made by the model that *did not train on it* (and `NaN` for rows in no test set), and `cvloss(x, y, mode, kind)` returns the loss: averaged over all test rows (`CVLossMode::Average`, one value per horizon) or fold by fold (`CVLossMode::Individual`). The loss is the mean squared error (`RegressionLoss::MSE`) or the mean absolute error (`MAE`).

**The Diebold–Mariano test.** Given the forecast errors `e_A` and `e_B` of two models on the same test points, it asks whether their expected losses differ. It works on the loss differences `d_t = L(e_{A,t}) − L(e_{B,t})`: if the models are equally good, the mean of `d_t` is zero. The statistic is that mean divided by its long-run standard error, which includes the autocorrelation of `d_t` (forecast errors from a multi-step forecast overlap, so they are correlated up to lag `h − 1`). With `hln_adjust` on (the default) it carries the Harvey–Leybourne–Newbold small-sample correction and is referred to a Student t distribution. `dm_test(err_a, err_b, DmOptions)` returns `stat`, `pvalue`, `critical`, `reject`, `mean_diff` and `lrv`; **a negative statistic favours the first model**. The options are `horizon` (the forecast horizon `h`), `max_lag`, `loss` (`Squared`, `Absolute` or `Power`), `hln_adjust`, `tail` and `alpha`.

The programs compare two direct forecasters on a simulated response: model A uses two lags of `y` only; model B adds two lags of an exogenous series `x` that really drives `y`.

## Loss per fold

The first program fits both models over an expanding-window partition with four test sets and prints the mean squared error in each fold. Fold-by-fold losses show whether an improvement is consistent or comes from one lucky period.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

struct Data { Series x, y; };

Data make_data(int n, std::uint64_t seed, const pluss::Config& cfg) {
  ts::ArimaFit white;
  const auto u = white.simulate(n, cfg, seed).host<double>();
  const auto e = white.simulate(n, cfg, seed + 1).host<double>();
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    x[t] = 0.8 * xp + 0.6 * u[t];
    y[t] = 0.5 * yp + 0.4 * xp + 0.3 * e[t];  // y depends on the PAST of x
    xp = x[t];
    yp = y[t];
  }
  return {pluss::make_series(x, {n, 1}).to(pluss::ScalarType::Float64), pluss::make_series(y, {n}).to(pluss::ScalarType::Float64)};
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const int n = 400;
  const auto data = make_data(n, 5, f64);
  const auto partition = ts::TsPartition::expanding_window(n, 4);

  ts::DirectForecasterOptions a, b;
  a.lag.horizon = b.lag.horizon = {1};
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  b.lag.predictor_lags = {{1, 2}};
  const auto cv_a = ts::DirectCVForecaster::fit(Series{}, data.y, partition, a, f64);
  const auto cv_b = ts::DirectCVForecaster::fit(data.x, data.y, partition, b, f64);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(28) << "mean squared error" << std::right << std::setw(10) << "fold 1" << std::setw(10) << "fold 2" << std::setw(10) << "fold 3" << std::setw(10) << "fold 4"
            << std::setw(10) << "average" << "\n";
  const auto fa = cv_a.cvloss(Series{}, data.y, ts::CVLossMode::Individual, ts::RegressionLoss::MSE, f64).host<double>();
  const auto fb = cv_b.cvloss(data.x, data.y, ts::CVLossMode::Individual, ts::RegressionLoss::MSE, f64).host<double>();
  const double avg_a = cv_a.cvloss(Series{}, data.y, ts::CVLossMode::Average, ts::RegressionLoss::MSE, f64).host<double>()[0];
  const double avg_b = cv_b.cvloss(data.x, data.y, ts::CVLossMode::Average, ts::RegressionLoss::MSE, f64).host<double>()[0];
  std::cout << std::left << std::setw(28) << "A: y lags only" << std::right;
  for (double v : fa) std::cout << std::setw(10) << v;
  std::cout << std::setw(10) << avg_a << "\n" << std::left << std::setw(28) << "B: y lags and x lags" << std::right;
  for (double v : fb) std::cout << std::setw(10) << v;
  std::cout << std::setw(10) << avg_b << "\n";
}
```

Output

```text
mean squared error              fold 1    fold 2    fold 3    fold 4   average
A: y lags only                  0.2183    0.1693    0.1514    0.1401    0.1698
B: y lags and x lags            0.1088    0.0867    0.0806    0.0803    0.0891
```

Compare the rows fold by fold. Model B, which uses the lags of the driver, has the lower error in every fold, and the average error is lower by a clear margin, so the improvement is consistent across the four test periods, which is much stronger evidence than the average alone. If B had won three folds and lost one by a lot, the average could mislead.

## Is the difference real?

An average difference in loss can come from chance: a finite test sample favours one model or the other by luck. The Diebold–Mariano test measures that. The program collects the out-of-sample errors of both models (the rows covered by a test set), and runs `dm_test` for A against B, for B against A (the statistic changes sign, the p-value does not), and for A against a third model A2 that differs from A only in using three lags instead of two (an alternative that should be indistinguishable).

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

struct Data { Series x, y; };

Data make_data(int n, std::uint64_t seed, const pluss::Config& cfg) {
  ts::ArimaFit white;
  const auto u = white.simulate(n, cfg, seed).host<double>();
  const auto e = white.simulate(n, cfg, seed + 1).host<double>();
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    x[t] = 0.8 * xp + 0.6 * u[t];
    y[t] = 0.5 * yp + 0.4 * xp + 0.3 * e[t];
    xp = x[t];
    yp = y[t];
  }
  return {pluss::make_series(x, {n, 1}).to(pluss::ScalarType::Float64), pluss::make_series(y, {n}).to(pluss::ScalarType::Float64)};
}

// Out-of-sample errors of a cross-validated direct forecaster, on the rows covered by a test set.
std::vector<double> errors(const Series& x, const Series& y, const ts::TsPartition& part, const ts::DirectForecasterOptions& o, const pluss::Config& cfg) {
  const auto cv = ts::DirectCVForecaster::fit(x, y, part, o, cfg);
  const auto pred = cv.cvpredict(x, y, cfg).reshape({y.numel()}).items<double>();
  const auto actual = y.host<double>();
  std::vector<double> e;
  for (std::size_t t = 0; t < actual.size(); ++t)
    if (std::isfinite(pred[t])) e.push_back(actual[t] - pred[t]);
  return e;
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const int n = 400;
  const auto data = make_data(n, 5, f64);
  const auto part = ts::TsPartition::expanding_window(n, 4);

  ts::DirectForecasterOptions a, b, a3;
  a.lag.horizon = b.lag.horizon = a3.lag.horizon = {1};
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  a3.lag.response_lags = {1, 2, 3};
  b.lag.predictor_lags = {{1, 2}};
  const auto ea = errors(Series{}, data.y, part, a, f64);
  const auto eb = errors(data.x, data.y, part, b, f64);
  const auto ea3 = errors(Series{}, data.y, part, a3, f64);
  const auto series = [&](const std::vector<double>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); };
  // The three error series cover slightly different rows; compare on the common last rows.
  const std::size_t m = std::min({ea.size(), eb.size(), ea3.size()});
  auto tail = [&](const std::vector<double>& v) { return std::vector<double>(v.end() - m, v.end()); };

  struct Pair { const char* name; std::vector<double> first, second; };
  const Pair pairs[] = {{"A vs B", tail(ea), tail(eb)}, {"B vs A", tail(eb), tail(ea)}, {"A vs A2 (three lags)", tail(ea), tail(ea3)}};
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "DM test on " << m << " out-of-sample errors (a negative statistic favours the first model)\n";
  std::cout << std::left << std::setw(24) << "comparison" << std::right << std::setw(14) << "mean diff" << std::setw(12) << "statistic" << std::setw(12) << "p-value" << "   decision\n";
  for (const auto& p : pairs) {
    const auto r = ts::dm_test(series(p.first), series(p.second), {}, f64);
    std::cout << std::left << std::setw(24) << p.name << std::right << std::setw(14) << r.mean_diff << std::setw(12) << r.stat << std::setw(12) << r.pvalue << (r.reject ? "   differ" : "   no difference") << "\n";
  }
}
```

Output

```text
DM test on 320 out-of-sample errors (a negative statistic favours the first model)
comparison                   mean diff   statistic     p-value   decision
A vs B                          0.0807      7.0063      0.0000   differ
B vs A                         -0.0807     -7.0063      0.0000   differ
A vs A2 (three lags)            0.0000      0.0019      0.9985   no difference
```

A against B has a positive mean loss difference (A is worse) and, because it is the first model in the comparison, a positive statistic and a very small p-value: B is better, and the difference is not chance. Swapping the order flips the sign and leaves the p-value unchanged, which is how to read the sign. The comparison of A with A2 gives a statistic near zero and a large p-value: the extra lag makes no detectable difference. A test that never says "no difference" would be useless; this one can.

## How often does the test fire when the models are equal?

A test is only useful if its error rate is what it claims. The program repeats the comparison over 60 simulated series for two cases: model A against A2 (equal in truth: any rejection is a false alarm, nominally 5%) and model A against B (a real difference: rejections measure power).

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

std::vector<double> errors(const Series& x, const Series& y, const ts::TsPartition& part, const ts::DirectForecasterOptions& o, const pluss::Config& cfg) {
  const auto cv = ts::DirectCVForecaster::fit(x, y, part, o, cfg);
  const auto pred = cv.cvpredict(x, y, cfg).reshape({y.numel()}).items<double>();
  const auto actual = y.host<double>();
  std::vector<double> e;
  for (std::size_t t = 0; t < actual.size(); ++t)
    if (std::isfinite(pred[t])) e.push_back(actual[t] - pred[t]);
  return e;
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const int n = 400, reps = 60;
  const auto part = ts::TsPartition::expanding_window(n, 4);
  ts::DirectForecasterOptions a, b, a3;
  a.lag.horizon = b.lag.horizon = a3.lag.horizon = {1};
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  a3.lag.response_lags = {1, 2, 3};
  b.lag.predictor_lags = {{1, 2}};

  int false_alarms = 0, detections = 0;
  for (int r = 0; r < reps; ++r) {
    ts::ArimaFit white;
    const auto u = white.simulate(n, f64, 3000 + 2 * r).host<double>();
    const auto e = white.simulate(n, f64, 3001 + 2 * r).host<double>();
    std::vector<double> x(n), y(n);
    double xp = 0.0, yp = 0.0;
    for (int t = 0; t < n; ++t) { x[t] = 0.8 * xp + 0.6 * u[t]; y[t] = 0.5 * yp + 0.4 * xp + 0.3 * e[t]; xp = x[t]; yp = y[t]; }
    const Series X = pluss::make_series(x, {n, 1}).to(pluss::ScalarType::Float64), Y = pluss::make_series(y, {n}).to(pluss::ScalarType::Float64);
    const auto ea = errors(Series{}, Y, part, a, f64), eb = errors(X, Y, part, b, f64), ea3 = errors(Series{}, Y, part, a3, f64);
    const std::size_t m = std::min({ea.size(), eb.size(), ea3.size()});
    auto s = [&](const std::vector<double>& v) { return pluss::make_series(std::vector<double>(v.end() - m, v.end()), {static_cast<std::int64_t>(m)}).to(pluss::ScalarType::Float64); };
    false_alarms += ts::dm_test(s(ea), s(ea3), {}, f64).reject;
    detections += ts::dm_test(s(ea), s(eb), {}, f64).reject;
  }
  std::cout << std::fixed << std::setprecision(3);
  std::cout << "A vs A2 (equal models):   rejected in " << static_cast<double>(false_alarms) / reps << " of the samples  (nominal 0.05)\n";
  std::cout << "A vs B (B is better):     rejected in " << static_cast<double>(detections) / reps << " of the samples\n";
}
```

Output

```text
A vs A2 (equal models):   rejected in 0.150 of the samples  (nominal 0.05)
A vs B (B is better):     rejected in 1.000 of the samples
```

The test detects the real difference in every one of the 60 samples, but the false-alarm rate is **15%**, three times the nominal 5% (with 60 samples a rate of 5% would show up as 3 ± 2 rejections; here there are 9). That is not a coding error; it is a known property of the Diebold–Mariano test when the two models are *nested*, as A and A2 are (A2 is A plus one more lag). The extra coefficient adds almost nothing to the forecast, so the two error series are nearly identical, the loss differences are tiny and their variance is estimated from a nearly degenerate sample, and the test over-rejects. Two lessons follow. First, trust the test most when comparing models that differ in kind (different predictors, different learners), where the error series genuinely differ. Second, for nested models do not read a marginal p-value (0.01 to 0.1) as evidence; require a clear difference in the cross-validated loss as well, and prefer the smaller model when the loss gain is within the fold-to-fold spread.

## Multi-step forecasts need the horizon option

For an `h`-step-ahead forecast, errors at neighbouring dates overlap (they share most of the forecast horizon), so they are autocorrelated up to lag `h − 1`. The Diebold–Mariano standard error must account for that, which is what `DmOptions::horizon` does. Leaving it at 1 for five-step forecasts treats overlapping errors as independent and makes the test too eager. The program compares the two models at horizon 5 with the option set correctly and incorrectly.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

std::vector<double> errors(const Series& x, const Series& y, const ts::TsPartition& part, const ts::DirectForecasterOptions& o, const pluss::Config& cfg) {
  const auto cv = ts::DirectCVForecaster::fit(x, y, part, o, cfg);
  const auto pred = cv.cvpredict(x, y, cfg).reshape({y.numel()}).items<double>();
  const auto actual = y.host<double>();
  std::vector<double> e;
  for (std::size_t t = 0; t < actual.size(); ++t)
    if (std::isfinite(pred[t])) e.push_back(actual[t] - pred[t]);
  return e;
}

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const int n = 400;
  ts::ArimaFit white;
  const auto u = white.simulate(n, f64, 5).host<double>();
  const auto e = white.simulate(n, f64, 6).host<double>();
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) { x[t] = 0.8 * xp + 0.6 * u[t]; y[t] = 0.7 * yp + 0.4 * xp + 0.3 * e[t]; xp = x[t]; yp = y[t]; }
  const Series X = pluss::make_series(x, {n, 1}).to(pluss::ScalarType::Float64), Y = pluss::make_series(y, {n}).to(pluss::ScalarType::Float64);

  const auto part = ts::TsPartition::expanding_window(n, 4);
  ts::DirectForecasterOptions a, b;
  a.lag.horizon = b.lag.horizon = {5};
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  b.lag.predictor_lags = {{1, 2}};
  const auto ea = errors(Series{}, Y, part, a, f64), eb = errors(X, Y, part, b, f64);
  const std::size_t m = std::min(ea.size(), eb.size());
  const auto s = [&](const std::vector<double>& v) { return pluss::make_series(std::vector<double>(v.end() - m, v.end()), {static_cast<std::int64_t>(m)}).to(pluss::ScalarType::Float64); };

  std::cout << std::fixed << std::setprecision(4) << "five-step forecasts, " << m << " out-of-sample errors\n";
  std::cout << std::left << std::setw(26) << "DmOptions::horizon" << std::right << std::setw(12) << "statistic" << std::setw(12) << "p-value" << std::setw(8) << "lags" << std::setw(14) << "long-run var" << "\n";
  for (int h : {1, 5}) {
    ts::DmOptions o;
    o.horizon = h;
    const auto r = ts::dm_test(s(ea), s(eb), o, f64);
    std::cout << std::left << std::setw(26) << (h == 1 ? "1 (wrong for 5-step)" : "5 (correct)") << std::right << std::setw(12) << r.stat << std::setw(12) << r.pvalue << std::setw(8) << r.lags
              << std::setw(14) << r.lrv << "\n";
  }
}
```

Output

```text
five-step forecasts, 320 out-of-sample errors
DmOptions::horizon           statistic     p-value    lags  long-run var
1 (wrong for 5-step)            4.0082      0.0001       0        0.2358
5 (correct)                     3.2686      0.0012       4        0.3457
```

With the horizon set to 5 the test uses four lags of the loss-difference autocorrelation, its long-run variance is larger, and the statistic is smaller in absolute value than with the horizon left at 1, so the p-value is larger. The wrong setting makes the evidence look stronger than it is. Set `horizon` to the forecast horizon of the errors you are comparing, every time.

## Decisions and pitfalls

- **Compare on the same test points.** Both models must be scored on exactly the same observations; trim the error series to the common rows, as the programs do.
- **Look at folds, not only the average.** Consistency across folds is evidence the margin is not luck.
- **Set `DmOptions::horizon` to the forecast horizon.** Otherwise a multi-step comparison is overconfident.
- **Choose the loss to match the use.** Squared error punishes large misses, absolute error is robust to outliers; test with the loss you will be judged by.
- **Be careful with nested models.** When one model is the other plus a few terms, the test rejects far too often (15% against 5% above); insist on a clear loss difference too.
- **A non-significant difference is not equality.** With little data the test cannot separate close models; then prefer the simpler one.

That completes the direct-forecasting chapter. [Chapter 7](../ts_07_plots.md) shows the correlation and residual plots that accompany these checks.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="direct_forecasts.html">
    <span class="label">Previous</span>
    <span class="title">Direct forecasts</span>
  </a>
</div>
<!-- apiref:nav:end -->
