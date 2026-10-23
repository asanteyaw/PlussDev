<!-- generated from ts_01_describing_a_series.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Time Series 1: Describing a Series

Before fitting any model, look at the series: how it moves, how each value relates to its past, and whether it is stationary. This chapter covers the transforms that reshape a series, the autocorrelation functions, and the tests for white noise, unit roots, stationarity and volatility clustering. After it you can compute the standard descriptive summaries and read the tests.

Every program is complete and its output is the output of that program. The two series used throughout are built from a fixed pseudo-random sequence, so the output is reproducible.

## Transforms

All transforms in `pluss::subsp::stats::ts` take a Series and return one of the same length, with `NaN` where the value is not defined.

| Function | Result |
|---|---|
| `diff(x, k)`, `returns(x, k)`, `log_returns(x, k)` | `x_t − x_{t−k}`, `x_t / x_{t−k} − 1`, `ln(x_t / x_{t−k})` |
| `lag(x, k)`, `lead(x, k)` | the series shifted back or forward |
| `rolling_mean`, `rolling_sum`, `rolling_min`, `rolling_max (x, window, min_periods)` | statistic over the trailing `window` observations, `NaN` until `min_periods` are available |
| `rolling_var`, `rolling_std (x, window, min_periods, ddof)` | sample variance / standard deviation (`ddof = 1`); `ddof = 0` gives the population one |
| `ema(x, alpha)` | exponential average `m_t = m_{t−1} + α (x_t − m_{t−1})`, `m_0 = x_0` |
| `ewm_std(x, alpha)` | the matching exponentially weighted standard deviation |
| `expanding_mean`, `expanding_std` | statistic over everything so far (`expanding_std` has `ddof`) |
| `zscore_rolling(x, window, ...)` | `(x_t − rolling mean) / rolling standard deviation` |
| `fill_gaps`, `resample` | fill missing values, aggregate to a coarser frequency |

- [The transforms](ts_01_describing_a_series/the_transforms.md)

## Autocorrelation

The autocorrelation at lag `k` is the correlation of the series with itself `k` steps back. The partial autocorrelation at lag `k` removes the effect of the lags in between. An AR(p) process has partial autocorrelations that vanish beyond lag `p`; an MA(q) process has autocorrelations that vanish beyond lag `q`. This is the classic way to read off a model's orders.

| Option | Meaning | Default |
|---|---|---|
| `max_lag` | highest lag returned | 20 |
| `norm` | `Biased` (divide the autocovariance by n) or `Unbiased` (by n − k) | `Biased` |
| `demean` | subtract the sample mean first | true |

- [ACF and PACF](ts_01_describing_a_series/acf_and_pacf.md)

## Tests

| Test | Null hypothesis | Rejecting means |
|---|---|---|
| `ljung_box(x, {lags, model_df})` | the autocorrelations up to each lag are all zero | the series is not white noise |
| `adf_test(y, {lags, model})` | unit root | the series is stationary |
| `kpss_test(y, {lags, trend})` | stationary (around a level or trend) | a unit root, or a trend not modelled |
| `arch_lm(x, {lags, demean})` | no volatility clustering | squared values are autocorrelated |

Because ADF and KPSS have opposite nulls, use both: a stationary series should reject ADF and keep KPSS; a unit-root series the reverse. If they disagree, the series may be near-integrated or the sample too short.

- [White noise, unit roots, stationarity](ts_01_describing_a_series/white_noise_unit_roots_stationarity.md)
- [Volatility clustering](ts_01_describing_a_series/volatility_clustering.md)

## Checklist

1. Plot nothing yet: compute the ACF and PACF and the tests; they tell you what to model.
2. Use ADF and KPSS together; difference a series that has a unit root.
3. For model residuals, set `model_df` in the Ljung–Box test.
4. Check squared residuals for ARCH effects before trusting a constant variance.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

ts_01_describing_a_series/the_transforms
ts_01_describing_a_series/acf_and_pacf
ts_01_describing_a_series/white_noise_unit_roots_stationarity
ts_01_describing_a_series/volatility_clustering
```
<!-- apiref:toctree:end -->
