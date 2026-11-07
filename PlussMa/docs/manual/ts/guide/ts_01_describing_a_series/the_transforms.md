<!-- generated from the_transforms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The transforms

[Back to Describing a Series](../ts_01_describing_a_series.md)

Most time-series work starts by reshaping the series: looking at changes instead of levels, comparing each value with its recent past, smoothing out noise, or repairing gaps. The functions in `pluss::subsp::stats::ts` that do this share one contract: they take a `Series` and return a `Series` **of the same length**, with `NaN` wherever the value is not defined. Keeping the length means the result lines up observation by observation with the input, so you can put a transform next to the original, subtract one from another, or feed it to a model without re-indexing. This page walks through the four groups of transforms: changes and shifts, rolling and expanding windows, exponential weighting, and gaps and frequency.

## Changes and shifts

`diff(x, k)` is `x_t − x_{t−k}`, `returns(x, k)` is `x_t / x_{t−k} − 1` and `log_returns(x, k)` is `ln(x_t / x_{t−k})`. The first `k` entries have nothing to compare against and are `NaN`. `lag(x, k)` and `lead(x, k)` simply shift the series backward or forward by `k`, which is what you use to build a regressor from a series' own past.

The program applies all of them to a short price series. Look at the `NaN` pattern on the left and right edges: a lag leaves the start undefined, a lead leaves the end undefined.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(4));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series p = pluss::make_series({100.0, 102.0, 101.0, 105.0, 103.0, 108.0}, f64);
  out.writeln("price", p.host<double>());
  out.writeln("diff(1)", ts::diff(p, 1).host<double>());
  out.writeln("diff(2)", ts::diff(p, 2).host<double>());
  out.writeln("returns(1)", ts::returns(p, 1).host<double>());
  out.writeln("log_returns(1)", ts::log_returns(p, 1).host<double>());
  out.writeln("lag(1)", ts::lag(p, 1).host<double>());
  out.writeln("lead(1)", ts::lead(p, 1).host<double>());
}
```

Output

```text
price [100.0000, 102.0000, 101.0000, 105.0000, 103.0000, 108.0000]
diff(1) [nan, 2.0000, -1.0000, 4.0000, -2.0000, 5.0000]
diff(2) [nan, nan, 1.0000, 3.0000, 2.0000, 3.0000]
returns(1) [nan, 0.0200, -0.0098, 0.0396, -0.0190, 0.0485]
log_returns(1) [nan, 0.0198, -0.0099, 0.0388, -0.0192, 0.0474]
lag(1) [nan, 100.0000, 102.0000, 101.0000, 105.0000, 103.0000]
lead(1) [102.0000, 101.0000, 105.0000, 103.0000, 108.0000, nan]
```

`returns` and `log_returns` are close for small moves and drift apart for large ones; the log return is the one that adds up over time (the log return over two days is the sum of the two daily log returns), which is why it is the usual input to volatility models. `diff` is the right transform when the series is already in the units you care about, or can be negative, where a ratio is meaningless.

Differencing is also how you remove a stochastic trend before fitting a stationary model. The second program differences a random walk once and shows what that does to the first-order autocorrelation: the walk is almost perfectly correlated with its own past, its changes are not.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  ts::ArimaFit noise;
  noise.sigma = 1.0;
  const Series shocks = noise.simulate(300, f64, /*seed=*/8);
  const Series walk = shocks.cumsum(0);
  const Series changes = ts::diff(walk, 1);

  ts::AcfOptions a;
  a.max_lag = 1;
  std::cout << std::fixed << std::setprecision(3);
  std::cout << "lag-1 autocorrelation of the walk:    " << ts::acf(walk, a).host<double>()[1] << "\n";
  // The first difference is NaN by construction, so drop it before measuring.
  std::vector<double> kept = changes.host<double>();
  kept.erase(kept.begin());
  const Series tail = pluss::make_series(kept, f64);
  std::cout << "lag-1 autocorrelation of its changes: " << ts::acf(tail, a).host<double>()[1] << "\n";
}
```

Output

```text
lag-1 autocorrelation of the walk:    0.973
lag-1 autocorrelation of its changes: -0.117
```

Notice the slice: because a transform returns a series of the same length, the leading `NaN` stays in the result and has to be removed before a statistic that cannot handle it. This is the price of the same-length contract and it is deliberate; you decide where the undefined region ends.

## Rolling and expanding windows

A rolling statistic summarises the trailing `window` observations: `rolling_mean`, `rolling_sum`, `rolling_min`, `rolling_max`, `rolling_var` and `rolling_std`. The `min_periods` argument controls how the start of the series is treated. With `min_periods = 1` the statistic is available from the first observation (computed on however many values exist), with `min_periods = window` the first `window − 1` entries are `NaN`, and anything between is a compromise. For the variance and standard deviation, `ddof` is the divisor correction: `1` (the default) is the sample statistic and `0` the population one; a window holding no more than `ddof` observations gives `NaN`.

`expanding_mean` and `expanding_std` are the same idea with a window that grows to include everything so far, and `zscore_rolling` standardises each observation by the rolling mean and standard deviation that precede and include it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = pluss::make_series({4.0, 6.0, 5.0, 9.0, 8.0, 7.0, 12.0}, f64);
  out.writeln("x", x.host<double>());
  out.writeln("rolling_mean(3, min 1)", ts::rolling_mean(x, 3, 1).host<double>());
  out.writeln("rolling_mean(3, min 3)", ts::rolling_mean(x, 3, 3).host<double>());
  out.writeln("rolling_std(3, min 2)", ts::rolling_std(x, 3, 2).host<double>());
  out.writeln("rolling_std(3, ddof 0)", ts::rolling_std(x, 3, 2, 0).host<double>());
  out.writeln("rolling_max(3, min 3)", ts::rolling_max(x, 3, 3).host<double>());
  out.writeln("expanding_mean", ts::expanding_mean(x, 1).host<double>());
  out.writeln("expanding_std(min 2)", ts::expanding_std(x, 2).host<double>());
  out.writeln("zscore_rolling(3, min 3)", ts::zscore_rolling(x, 3, 3).host<double>());
}
```

Output

```text
x [4.000, 6.000, 5.000, 9.000, 8.000, 7.000, 12.000]
rolling_mean(3, min 1) [4.000, 5.000, 5.000, 6.667, 7.333, 8.000, 9.000]
rolling_mean(3, min 3) [nan, nan, 5.000, 6.667, 7.333, 8.000, 9.000]
rolling_std(3, min 2) [nan, 1.414, 1.000, 2.082, 2.082, 1.000, 2.646]
rolling_std(3, ddof 0) [nan, 1.000, 0.816, 1.700, 1.700, 0.816, 2.160]
rolling_max(3, min 3) [nan, nan, 6.000, 9.000, 9.000, 9.000, 12.000]
expanding_mean [4.000, 5.000, 5.000, 6.000, 6.400, 6.500, 7.286]
expanding_std(min 2) [nan, 1.414, 1.000, 2.160, 2.074, 1.871, 2.690]
zscore_rolling(3, min 3) [nan, nan, 0.000, 1.121, 0.320, -1.000, 1.134]
```

Pick the window by what you want to be robust to. A short window reacts quickly and is noisy; a long one is smooth and late. Choosing `min_periods` equal to the window is the honest option when a half-filled window would mislead, and `min_periods = 1` is convenient when you would rather have a rough number than none. The expanding versions never forget, so they converge to the full-sample statistic; compare them with a rolling statistic to see whether the series' level or spread is drifting.

## Exponential weighting

A rolling mean gives every observation in the window the same weight and none outside it. An exponentially weighted average gives recent observations more weight and forgets old ones smoothly: `ema(x, alpha)` is `m_t = m_{t−1} + α (x_t − m_{t−1})` started at `m_0 = x_0`, and `ewm_mean` is its alias under the name used for the companion `ewm_std`, which is the matching exponentially weighted standard deviation. The smoothing constant `alpha` is between 0 and 1; larger values track the series more closely, smaller values smooth more. A useful rule of thumb is that `alpha` corresponds to an average age of about `1/alpha − 1` observations.

The program feeds a step (a series that jumps from 0 to 1 and stays there) through three different `alpha` values. The response of the average to the step is the clearest picture of what `alpha` means.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::vector<double> v(12, 0.0);
  for (std::size_t i = 4; i < v.size(); ++i) v[i] = 1.0;
  const Series step = pluss::make_series(v, f64);
  out.writeln("step", step.host<double>());
  for (double alpha : {0.2, 0.5, 0.8}) out.writeln("ema alpha " + std::to_string(alpha).substr(0, 3), ts::ema(step, alpha).host<double>());
}
```

Output

```text
step [0.000, 0.000, 0.000, 0.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000]
ema alpha 0.2 [0.000, 0.000, 0.000, 0.000, 0.200, 0.360, 0.488, 0.590, 0.672, 0.738, 0.790, 0.832]
ema alpha 0.5 [0.000, 0.000, 0.000, 0.000, 0.500, 0.750, 0.875, 0.938, 0.969, 0.984, 0.992, 0.996]
ema alpha 0.8 [0.000, 0.000, 0.000, 0.000, 0.800, 0.960, 0.992, 0.998, 1.000, 1.000, 1.000, 1.000]
```

After a step, the average closes a fraction `alpha` of the remaining gap at every observation, so with `alpha = 0.2` it needs several steps to get most of the way, and with `alpha = 0.8` it is there almost at once. This is the knob that trades lag against noise, and it is why exponential weighting is the standard choice for a running level or a running volatility estimate that must adapt without storing a window.

## Gaps and frequency

Real series have missing values and arrive at the wrong frequency. `fill_gaps(x, mode)` repairs `NaN` entries: `Forward` carries the last observed value forward, `Backward` pulls the next one back, `Zero` and `Constant` insert a fixed value. `resample(timeline, values, freq, op)` aggregates observations into coarser bins (for example, intraday readings to days) with `Mean`, `Sum`, `Min`, `Max`, `First`, `Last` or `Count`; it needs the `Timeline` that labels the observations, and it returns both the bin labels and the aggregated values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  pluss::tils::OutputEngine out;
  out.print_defaults(pluss::tils::PrintOptions{}.fixed(2));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double nan = std::nan("");
  const Series x = pluss::make_series({1.0, nan, nan, 4.0, nan, 6.0}, f64);
  out.writeln("with gaps", x.host<double>());
  out.writeln("forward", ts::fill_gaps(x, ts::GapFillMode::Forward).host<double>());
  out.writeln("backward", ts::fill_gaps(x, ts::GapFillMode::Backward).host<double>());
  out.writeln("zero", ts::fill_gaps(x, ts::GapFillMode::Zero).host<double>());
  out.writeln("constant -1", ts::fill_gaps(x, ts::GapFillMode::Constant, -1.0).host<double>());

  // Five readings over three days, aggregated to one value per day.
  const auto timeline = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-01T12:00:00Z", "2024-01-02T00:00:00Z",
                                              "2024-01-02T12:00:00Z", "2024-01-03T06:00:00Z"});
  const Series readings = pluss::make_series({1.0, 3.0, 2.0, 4.0, 10.0}, f64);
  std::cout << "\n";
  out.writeln("readings", readings.host<double>());
  out.writeln("daily mean", (ts::resample(timeline, readings, "D", ts::ResampleOp::Mean).values).host<double>());
  out.writeln("daily sum", (ts::resample(timeline, readings, "D", ts::ResampleOp::Sum).values).host<double>());
  out.writeln("daily count", (ts::resample(timeline, readings, "D", ts::ResampleOp::Count).values).host<double>());
}
```

Output

```text
with gaps [1.00, nan, nan, 4.00, nan, 6.00]
forward [1.00, 1.00, 1.00, 4.00, 4.00, 6.00]
backward [1.00, 4.00, 4.00, 4.00, 6.00, 6.00]
zero [1.00, 0.00, 0.00, 4.00, 0.00, 6.00]
constant -1 [1.00, -1.00, -1.00, 4.00, -1.00, 6.00]

readings [1.00, 3.00, 2.00, 4.00, 10.00]
daily mean [2.00, 3.00, 10.00]
daily sum [4.00, 6.00, 10.00]
daily count [2.00, 2.00, 1.00]
```

Which fill is right depends on what the gap means. Carrying the last value forward is the natural choice for a state that persists (a price between trades) and uses only past information, so it is safe in a forecasting setting; a backward fill uses the future and leaks it into the past, so avoid it when the result feeds a model that predicts ahead. Filling with zero is only right when absence really means "nothing happened" (a count), and a constant is for sentinel values. For a series that is genuinely irregular, resample to a regular grid first and then decide how to treat the empty bins, which stay in the result as `NaN` (or 0 for a count) so that you can see them.

## Decisions and pitfalls

- **Transforms never drop observations**, they mark them `NaN`. Slice off the undefined region before an autocorrelation, a test or a model fit.
- **Do not fill and then difference carelessly.** A forward-filled gap becomes a run of zero changes, which understates volatility; compute statistics on the observed values or fill after you have thought about what the gap means.
- **Use log returns for compounding, simple returns for portfolios.** Log returns add across time; simple returns add across assets.
- **A rolling statistic at the start of the series is not comparable** with one deep inside it unless `min_periods` equals the window.

Where next: [ACF and PACF](acf_and_pacf.md) measures how an observation depends on its own past, which is what tells you which of these transforms (and later which model) to use.
