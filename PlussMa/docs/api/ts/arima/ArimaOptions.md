<!-- generated from ArimaOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::ArimaOptions`

Options of `ts::Arima::fit_mle`: the orders of the model, starting values and search settings.

`ArimaOptions` names the regular orders `(ar_order, diff_order, ma_order)`, the seasonal orders and period, and how the likelihood is maximised.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `ar_order` | `std::int64_t` | 1 | Number of autoregressive coefficients `p`. |
| `diff_order` | `std::int64_t` | 0 | Number of regular differences `d` taken before fitting. |
| `ma_order` | `std::int64_t` | 0 | Number of moving-average coefficients `q`. |
| `seasonal_ar_order` | `std::int64_t` | 0 | Number of seasonal autoregressive coefficients `P` (at multiples of the period). |
| `seasonal_diff_order` | `std::int64_t` | 0 | Number of seasonal differences `D`. |
| `seasonal_ma_order` | `std::int64_t` | 0 | Number of seasonal moving-average coefficients `Q`. |
| `seasonal_period` | `std::int64_t` | 0 | The season length `s`; it must exceed 1 when any seasonal order is used. |
| `demean` | `bool` | true | Subtract the sample mean first; a further free constant (`intercept`) is estimated as well. |
| `intercept0` | `double` | 0 | Starting value of the intercept. |
| `ar0, ma0, sar0, sma0` | `std::vector<double>` | empty | Starting values of the four coefficient groups; empty means the zero polynomial, which is valid. |
| `beta0` | `std::vector<double>` | empty | Starting values of the regression coefficients when exogenous regressors are used. |
| `variance_floor` | `double` | 1e-12 | Smallest innovation variance the search accepts. |
| `stationarity_margin` | `double` | 1e-6 | The partial autocorrelations are kept inside `±(1 − margin)`. |
| `finite_diff_eps` | `double` | 1e-5 | Step of the numerical derivatives used by `score` and `observed_inf`. |
| `max_iterations, max_evaluations` | `std::int64_t` | 800, 6000 | Limits of the Nelder–Mead search. |
| `simplex_delta` | `double` | 0.05 | Relative size of the initial simplex. |
| `exact_likelihood` | `bool` | true | `true`: the exact Gaussian likelihood (Kalman filter, stationary start). `false`: the conditional likelihood that skips the first `max(p, q)` observations. Models with exogenous regressors always use the conditional likelihood. |

## Guidance

Choose the orders from the autocorrelation and partial autocorrelation functions and confirm by comparing log-likelihoods and information criteria of neighbouring models. Starting values rarely matter: the search is over a parametrisation in which every candidate is valid, and the zero start is well inside it.

## See Also

- [`Arima`](Arima.md)
- [`ArimaFit`](ArimaFit.md)
- [`SarimaOptions`](SarimaOptions.md)
