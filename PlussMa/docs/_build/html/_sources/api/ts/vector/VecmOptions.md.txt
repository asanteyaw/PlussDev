<!-- generated from VecmOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::VecmOptions`

Options of `ts::Vecm::fit_mle`.

`VecmOptions` chooses the cointegration rank, the lag order and the deterministic terms.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lag_order` | `std::int64_t` | 1 | Order `p` of the VAR in levels; the model has `p − 1` lagged differences. |
| `rank` | `std::int64_t` | −1 | The cointegration rank `r`; a negative value selects it by the trace test at level `alpha`. |
| `include_intercept` | `bool` | true | An unrestricted constant. |
| `include_trend` | `bool` | false | An unrestricted trend. |
| `alpha` | `double` | 0.05 | Level of the trace test when the rank is selected automatically. |
| `ridge` | `double` | 0 | Ridge penalty used when the rank equals the number of series (unrestricted least squares). |
| `pinv_rcond` | `double` | 1e-12 | Cutoff of the pseudo-inverse in the least-squares steps. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`Vecm`](Vecm.md)
