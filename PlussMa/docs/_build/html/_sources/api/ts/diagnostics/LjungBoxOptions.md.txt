<!-- generated from LjungBoxOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::LjungBoxOptions`

Options of `ts::ljung_box`.

`LjungBoxOptions` selects the lag sets to test and the correction for fitted model coefficients.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lags` | `std::vector<std::int64_t>` | `{10}` | The lag sets: one test is run for each, using autocorrelations at lags 1 up to that value. |
| `model_df` | `std::int64_t` | 0 | Degrees of freedom to subtract: the number of ARMA coefficients estimated when the input is a residual series. |
| `norm` | `ts::AcfNorm` | `Biased` | Normalisation of the autocovariances, as in `AcfOptions`. |
| `demean` | `bool` | true | Subtract the sample mean first; turn off for residuals whose mean is zero by construction. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`ljung_box`](ljung_box.md)
