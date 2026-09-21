# `classic::EstimatorBase` Introduction

## Role

`EstimatorBase` is the polymorphic base every classifier and regressor family
implements. You never see the name `EstimatorBase` in your own variable
declarations — you hold a `ClassifierBase&` / `RegressorBase&` (or the
owning `std::unique_ptr<ClassifierBase>` / `std::unique_ptr<RegressorBase>`
[`make_classifier`](../make_classifier.md) / [`make_regressor`](../make_regressor.md)
return) — but every method on this page is reached through one of those two
types, inherited unchanged.

## Lifecycle

An estimator obtained from `make_classifier` / `make_regressor` starts
untrained (`trained()` is `false`). Exactly one sequence of calls moves it
through its lifecycle:

1. **`learn(batch, options)`** — fit. Returns a `Status`; check `.ok()` before
   trusting `trained()` afterward.
2. **`infer(batch, options)`** (classification: hard labels;
   `ClassifierBase::infer_scores` for raw scores) / **`assess(batch, options)`**
   (score against known labels) — any number of times, in any order, once
   trained. Calling either before `learn` succeeds returns a failure
   `Status`/`StatusOr`.
3. **`dump_state()`** / **`load_state(state)`** — optional persistence, used
   internally by [`io::save`](../../io/save.md) / [`io::load`](../../io/load.md)
   rather than called directly in most code.

Calling `learn` again on an already-trained estimator re-fits from scratch —
see [`learn`](learn.md) for exactly what state that discards.

## Related Pages

- [`ClassifierBase`](../ClassifierBase.md) — adds `infer_scores` to this
  contract.
- [`RegressorBase`](../RegressorBase.md) — this contract, unchanged.
- [`core`](../../core.md) — `TrainOptions` / `InferOptions` / `AssessOptions`,
  the configuration every method here takes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
