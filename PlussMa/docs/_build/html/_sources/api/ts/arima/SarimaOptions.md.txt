<!-- generated from SarimaOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::SarimaOptions`

Options of `ts::Sarima::fit_mle`: the orders in the usual seasonal letters.

`SarimaOptions` is `ArimaOptions` spelled with `(p, d, q)(P, D, Q)s`.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `p, d, q` | `std::int64_t` | 1, 0, 0 | Regular autoregressive, differencing and moving-average orders. |
| `P, D, Q` | `std::int64_t` | 0, 0, 0 | Seasonal autoregressive, differencing and moving-average orders. |
| `s` | `std::int64_t` | 0 | The season length; it must exceed 1 when `P`, `D` or `Q` is nonzero. |
| `demean` | `bool` | true | Subtract the sample mean first. |
| `intercept0, ar0, ma0, sar0, sma0, beta0` | `double / vectors` | 0 / empty | Starting values. |
| `variance_floor, stationarity_margin, finite_diff_eps` | `double` | 1e-12, 1e-6, 1e-5 | As in `ArimaOptions`. |
| `max_iterations, max_evaluations, simplex_delta` | `—` | 800, 6000, 0.05 | Search limits, as in `ArimaOptions`. |
| `exact_likelihood` | `bool` | true | Exact (Kalman) or conditional likelihood, as in `ArimaOptions`. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`Sarima`](Sarima.md)
- [`ArimaOptions`](ArimaOptions.md)
