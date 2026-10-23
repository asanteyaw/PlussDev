<!-- generated from JohansenResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::JohansenResult`

Result of `ts::johansen_test`.

`JohansenResult` holds one entry per hypothesis `rank ≤ r`, `r = 0 … k − 1`.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `eigenvalues` | `pluss::Series` | — | The squared canonical correlations, in decreasing order. |
| `stat` | `pluss::Series` | — | The test statistic for `rank ≤ r`. |
| `pvalue` | `pluss::Series` | — | From the simulated asymptotic distribution. |
| `critical` | `pluss::Series` | — | Critical value at `alpha`. |
| `reject` | `pluss::Series` | — | 1 where `rank ≤ r` is rejected. |
| `dof` | `pluss::Series` | — | Informational: `m` (max-eigenvalue) or `m²` (trace) with `m = k − r`. |
| `alpha` | `pluss::Series` | — | Adjustment loadings for the selected rank (`k × rank`). |
| `beta` | `pluss::Series` | — | Cointegrating vectors for the selected rank (`k × rank`, eigenvectors normalised to `β' S11 β = I`). |
| `selected_rank` | `std::int64_t` | — | The first `r` that is not rejected, at most `k − 1`. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`johansen_test`](johansen_test.md)
