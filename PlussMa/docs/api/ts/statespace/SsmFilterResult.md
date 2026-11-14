<!-- generated from SsmFilterResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::SsmFilterResult`

Result of `Ssm::filter`.

`SsmFilterResult` is plain data; each field is a `Series`.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `filtered_states` | `pluss::Series` | — | `T × m`: the state estimate given the data up to each time. |
| `filtered_states_cov` | `pluss::Series` | — | `T × m × m`: its covariance. |
| `forecasted_states` | `pluss::Series` | — | `T × m`: the prediction of the state at each time made one step earlier. |
| `forecasted_states_cov` | `pluss::Series` | — | `T × m × m`: its covariance. |
| `forecasted_obs` | `pluss::Series` | — | `T × n`: the one-step forecast of each observation. |
| `forecasted_obs_cov` | `pluss::Series` | — | `T × n × n`: its covariance. |
| `forecast_obs_error` | `pluss::Series` | — | `T × n`: observation minus forecast. |
| `kalman_gain` | `pluss::Series` | — | `T × m × n`: the gain. |
| `data_used` | `pluss::Series` | — | `T × n`: 1 where the observation was used, 0 where it was missing. |
| `loglik_per_obs` | `pluss::Series` | — | `T`: the log-likelihood contribution of each time. |
| `loglik` | `double` | — | The total log-likelihood. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`Ssm`](Ssm.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Ssm.html">
    <span class="label">Previous</span>
    <span class="title">Ssm</span>
  </a>
  <a class="next-link" href="SsmForecastResult.html">
    <span class="label">Next</span>
    <span class="title">SsmForecastResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
