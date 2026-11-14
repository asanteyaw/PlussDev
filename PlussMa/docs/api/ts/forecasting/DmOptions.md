<!-- generated from DmOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::DmOptions`

Options of `ts::dm_test`.

`DmOptions` selects the loss, the horizon of the forecasts and the form of the test.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `horizon` | `std::int64_t` | 1 | The forecast horizon `h`: the errors are correlated up to lag `h − 1`, which the long-run variance accounts for. |
| `max_lag` | `std::int64_t` | −1 | Lags of the long-run variance; −1 means `horizon − 1`. |
| `loss` | `ts::DmLoss` | `Squared` | `Squared`, `Absolute` or `Power` (with `loss_power`). |
| `loss_power` | `double` | 2 | The exponent of the `Power` loss. |
| `hln_adjust` | `bool` | true | Apply the Harvey–Leybourne–Newbold small-sample correction and use a Student t reference; otherwise a normal. |
| `tail` | `ts::DmTail` | `Both` | `Both`, `Left` (first model better) or `Right`. |
| `alpha` | `double` | 0.05 | Significance level. |

## Guidance

The defaults are a sensible starting point; change a field only for the reason its row gives.

## See Also

- [`dm_test`](dm_test.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dm_test.html">
    <span class="label">Previous</span>
    <span class="title">dm_test</span>
  </a>
  <a class="next-link" href="DmResult.html">
    <span class="label">Next</span>
    <span class="title">DmResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
