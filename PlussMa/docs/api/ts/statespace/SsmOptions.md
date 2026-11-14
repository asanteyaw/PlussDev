<!-- generated from SsmOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::SsmOptions`

Options of the state-space filter and smoother.

`SsmOptions` has one setting.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `tolerance` | `double` | 0 | An observation channel whose forecast variance is at or below this value is ignored in the update (it carries no information). |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`Ssm`](Ssm.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SsmModel.html">
    <span class="label">Previous</span>
    <span class="title">SsmModel</span>
  </a>
  <a class="next-link" href="SsmSimResult.html">
    <span class="label">Next</span>
    <span class="title">SsmSimResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
