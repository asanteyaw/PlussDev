<!-- generated from DirectStepModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::DirectStepModel`

The model fitted for one horizon of a direct forecaster.

`DirectStepModel` is plain data returned by `DirectForecaster::step`.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `horizon` | `std::int64_t` | 1 | The horizon this model forecasts. |
| `active_columns_index` | `pluss::Series` | — | The feature columns used at this horizon. |
| `beta` | `pluss::Series` | — | The coefficients: the intercept first (when fitted), then one per active feature in the order non-leading predictor lags, response lags, leading predictor lags. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`DirectForecaster`](DirectForecaster.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DirectForecasterOptions.html">
    <span class="label">Previous</span>
    <span class="title">DirectForecasterOptions</span>
  </a>
  <a class="next-link" href="dm_test.html">
    <span class="label">Next</span>
    <span class="title">dm_test</span>
  </a>
</div>
<!-- apiref:nav:end -->
