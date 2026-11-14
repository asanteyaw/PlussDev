<!-- generated from GrangerResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::GrangerResult`

Result of `VarFit::granger_test`.

`GrangerResult` holds one entry per test, each field a `Series` of that length.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `cause` | `pluss::Series` | — | The index of the tested cause; −1 for a joint test of several series. |
| `effect` | `pluss::Series` | — | The index of the effect equation. |
| `stat` | `pluss::Series` | — | The statistic. |
| `pvalue` | `pluss::Series` | — | Its upper-tail probability under the null that the cause's lags have zero coefficients. |
| `critical` | `pluss::Series` | — | The critical value at `alpha`. |
| `dof1` | `pluss::Series` | — | Number of restricted coefficients. |
| `dof2` | `pluss::Series` | — | Residual degrees of freedom of the equation (F form and reference distribution). |
| `reject` | `pluss::Series` | — | 1 where the null is rejected. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`GrangerOptions`](GrangerOptions.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GrangerOptions.html">
    <span class="label">Previous</span>
    <span class="title">GrangerOptions</span>
  </a>
  <a class="next-link" href="IrfOptions.html">
    <span class="label">Next</span>
    <span class="title">IrfOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
