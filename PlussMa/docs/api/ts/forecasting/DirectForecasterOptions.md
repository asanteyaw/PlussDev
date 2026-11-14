<!-- generated from DirectForecasterOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::DirectForecasterOptions`

Options of `DirectForecaster::fit` and `DirectCVForecaster::fit`.

`DirectForecasterOptions` holds the lag specification and the learner.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lag` | `ts::LagPlanOptions` | defaults | Horizons and lags; see [`LagPlanOptions`](LagPlanOptions.md). |
| `learner` | `ts::DirectLearnerKind` | `OLS` | `OLS` or `Ridge`. |
| `fit_intercept` | `bool` | true | Add a constant to every regression. |
| `ridge_lambda` | `double` | 1e-6 | Penalty of the ridge learner (the intercept is not penalised). |
| `pinv_rcond` | `double` | 1e-12 | Relative cutoff of the pseudo-inverse. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`DirectForecaster`](DirectForecaster.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DirectForecaster.html">
    <span class="label">Previous</span>
    <span class="title">DirectForecaster</span>
  </a>
  <a class="next-link" href="DirectStepModel.html">
    <span class="label">Next</span>
    <span class="title">DirectStepModel</span>
  </a>
</div>
<!-- apiref:nav:end -->
