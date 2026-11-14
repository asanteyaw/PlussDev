<!-- generated from SsmSmoothResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::SsmSmoothResult`

Result of `Ssm::smooth`.

`SsmSmoothResult` is plain data; each field is a `Series`.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `filter` | `ts::SsmFilterResult` | — | The filter result. |
| `smoothed_states` | `pluss::Series` | — | `T × m`: the state estimate given *all* the data. |
| `smoothed_states_cov` | `pluss::Series` | — | `T × m × m`: its covariance. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`Ssm`](Ssm.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SsmSimResult.html">
    <span class="label">Previous</span>
    <span class="title">SsmSimResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
