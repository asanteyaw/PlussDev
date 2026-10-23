<!-- generated from ArchLmResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::ArchLmResult`

Result of `ts::arch_lm`.

`ArchLmResult` carries the test statistic and what is needed to read it.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `stat` | `double` | — | The statistic `n R²` of the regression of squares on their lags. |
| `pvalue` | `double` | — | Upper-tail chi-squared probability. |
| `critical` | `double` | — | Chi-squared quantile at `1 − alpha`. |
| `reject` | `bool` | — | True when the statistic exceeds the critical value: volatility clustering is present. |
| `dof` | `std::int64_t` | — | Degrees of freedom (the lags). |
| `effective_n` | `std::int64_t` | — | Observations in the regression: the length minus the lags. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`arch_lm`](arch_lm.md)
