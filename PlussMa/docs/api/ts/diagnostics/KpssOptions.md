<!-- generated from KpssOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::KpssOptions`

Options of `ts::kpss_test`.

`KpssOptions` chooses the deterministic part and the bandwidth of the long-run variance.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lags` | `std::int64_t` | 0 | Bandwidth of the Bartlett-weighted long-run variance; 0 uses the plain variance and suits only uncorrelated errors. |
| `trend` | `bool` | true | Test stationarity around a linear trend (true) or around a constant level (false). |
| `alpha` | `double` | 0.05 | Significance level of the critical value and of `reject`. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`kpss_test`](kpss_test.md)
