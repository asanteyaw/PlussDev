<!-- generated from LjungBoxResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::LjungBoxResult`

Result of `ts::ljung_box`.

`LjungBoxResult` holds one entry per element of `LjungBoxOptions::lags`, each field a `Series` of that length.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lags` | `pluss::Series` | — | The lag sets that were tested. |
| `stat` | `pluss::Series` | — | The Ljung–Box statistic for each. |
| `pvalue` | `pluss::Series` | — | The upper-tail chi-squared probability of each statistic (small: not white noise). |
| `dof` | `pluss::Series` | — | The degrees of freedom: lags minus `model_df`. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`ljung_box`](ljung_box.md)
