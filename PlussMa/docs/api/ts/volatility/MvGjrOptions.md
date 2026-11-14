<!-- generated from MvGjrOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::MvGjrOptions`

Options of `ts::MvGjr::fit_mle`.

`MvGjrOptions` holds the options of the univariate model fitted to every series, and the settings of the correlation estimate.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `marginal` | `ts::GjrOptions` | defaults | Options of the GJR-GARCH fitted to each column; see [`GjrOptions`](GjrOptions.md). |
| `corr_shrinkage` | `double` | 1e-6 | Weight in [0, 1] of the identity matrix mixed into the estimated correlation (shrinks it towards no correlation). |
| `corr_jitter` | `double` | 1e-9 | Added to the diagonal until the correlation matrix is positive definite. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`MvGjr`](MvGjr.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MvGjrFit.html">
    <span class="label">Previous</span>
    <span class="title">MvGjrFit</span>
  </a>
</div>
<!-- apiref:nav:end -->
