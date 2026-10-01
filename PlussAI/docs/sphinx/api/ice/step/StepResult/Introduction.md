# `pluss::ai::ice::StepResult` Introduction

`StepResult` owns exactly two fields -- `predictions` and `loss` -- and
exists because the four [`Model`](../../model/Model.md) step methods
([`learn_step`](../../model/Model.md)/[`valid_step`](../../model/Model.md)/
[`test_step`](../../model/Model.md)/[`infer_step`](../../model/Model.md))
don't all produce the same thing: `infer_step` has no target to compute
a loss against, so it leaves `loss` undefined; the other three set both.

## Reading A Result

Check [`has_predictions`](has_predictions.md)/[`has_loss`](has_loss.md)
rather than assuming either field is set -- which step method produced
the result determines what to expect, but a custom override is free to
leave either undefined too (for example, a model that computes `loss`
lazily only when actually needed).

## Related Pages

- [`Model`](../../model/Model.md) -- the four step methods that return
  this.
- [`Batch`](../Batch.md) / [`StepContext`](../StepContext.md) -- what a
  step method reads to produce this.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="has_loss.html">
    <span class="label">Next</span>
    <span class="title">has_loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
