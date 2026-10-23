# PlussMa `ts`

`ts` is PlussMa's time-series framework: the transforms, tests, models and forecasters that work on an ordered series or a matrix of series.

## Public Positioning

Reach for `ts` when the order of the observations matters. For an independent sample use the distribution and inference frameworks.

## Include Surface

```cpp
#include <pluss/ma.h>
```

## Public Namespace

```cpp
namespace ts = pluss::subsp::stats::ts;
```

Prose and signatures on this reference spell the namespace out as `ts::`; runnable examples declare the alias above.

## Families

| Family | What it owns |
|---|---|
| [`transforms`](ts/transforms.md) | changes, shifts, rolling and exponentially weighted statistics, gap filling |
| [`diagnostics`](ts/diagnostics.md) | autocorrelation, partial autocorrelation and the Ljung-Box, ADF, KPSS and ARCH-LM tests |
| [`arima`](ts/arima.md) | ARIMA and seasonal ARIMA models fitted by exact maximum likelihood |
| [`volatility`](ts/volatility.md) | GARCH, GJR and EGARCH variance models and their constant-correlation multivariate forms |
| [`vector`](ts/vector.md) | vector autoregressions, impulse responses, Granger tests, cointegration and error-correction models |
| [`statespace`](ts/statespace.md) | the Kalman filter, smoother, forecasts and simulation for linear Gaussian state-space models |
| [`forecasting`](ts/forecasting.md) | time-series partitions, direct multi-horizon forecasters, cross-validated loss and the Diebold-Mariano test |
| [`plot`](ts/plot.md) | autocorrelation and residual plots and their payloads |

```{toctree}
:maxdepth: 3
:hidden:

ts/transforms
ts/diagnostics
ts/arima
ts/volatility
ts/vector
ts/statespace
ts/forecasting
ts/plot
```
