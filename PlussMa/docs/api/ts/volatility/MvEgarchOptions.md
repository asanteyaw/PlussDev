<!-- generated from MvEgarchOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::MvEgarchOptions`

Options of `ts::MvEgarch::fit_mle`.

`MvEgarchOptions` holds the options of the univariate model fitted to every series, and the settings of the correlation estimate.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `marginal` | `ts::EgarchOptions` | defaults | Options of the EGARCH fitted to each column; see [`EgarchOptions`](EgarchOptions.md). |
| `corr_shrinkage` | `double` | 1e-6 | Weight in [0, 1] of the identity matrix mixed into the estimated correlation (shrinks it towards no correlation). |
| `corr_jitter` | `double` | 1e-9 | Added to the diagonal until the correlation matrix is positive definite. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`MvEgarch`](MvEgarch.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MvEgarchFit.html">
    <span class="label">Previous</span>
    <span class="title">MvEgarchFit</span>
  </a>
  <a class="next-link" href="MvGarch.html">
    <span class="label">Next</span>
    <span class="title">MvGarch</span>
  </a>
</div>
<!-- apiref:nav:end -->
