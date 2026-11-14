<!-- generated from VarOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::VarOptions`

Options of `ts::Var::fit_mle`.

`VarOptions` chooses the lag order and the deterministic terms of the regression.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `lag_order` | `std::int64_t` | 1 | The order `p`: how many lags of every series enter each equation. |
| `include_intercept` | `bool` | true | Include a constant in every equation. |
| `include_trend` | `bool` | false | Include a linear time trend in every equation. |
| `ridge` | `double` | 0 | Ridge penalty on the lag coefficients (not on the deterministic terms); 0 gives plain least squares. |
| `pinv_rcond` | `double` | 1e-12 | Relative cutoff of the pseudo-inverse used for the least-squares solution. |

## Guidance

Choose `lag_order` by comparing Akaike's `2m − 2 loglik` or Schwarz's `m log T − 2 loglik` across orders, with `m = k (k p + 1)` parameters for `k` series and an intercept.

## See Also

- [`Var`](Var.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="VarFit.html">
    <span class="label">Previous</span>
    <span class="title">VarFit</span>
  </a>
  <a class="next-link" href="Vecm.html">
    <span class="label">Next</span>
    <span class="title">Vecm</span>
  </a>
</div>
<!-- apiref:nav:end -->
