<!-- generated from IrfOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::IrfOptions`

Options of `VarFit::irf`.

`IrfOptions` selects the horizon and the way shocks are identified.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `horizon` | `std::int64_t` | 20 | Number of steps of the response, the impact (step 0) included; it must be greater than 0. |
| `method` | `ts::IrfMethod` | `Orthogonalized` | `Orthogonalized`: shocks made uncorrelated by the Cholesky factor of the covariance, so the order of the series matters (the first does not respond to the others within the period). `Generalized`: each series shocked alone, letting the others respond through the correlation of the shocks; independent of the ordering. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`VarFit`](VarFit.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GrangerResult.html">
    <span class="label">Previous</span>
    <span class="title">GrangerResult</span>
  </a>
  <a class="next-link" href="johansen_test.html">
    <span class="label">Next</span>
    <span class="title">johansen_test</span>
  </a>
</div>
<!-- apiref:nav:end -->
