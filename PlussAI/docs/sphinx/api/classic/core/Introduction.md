# `classic::core` Introduction

## What This Family Owns

`core` owns two things every other family depends on:

1. **How a `classic` call reports failure.** `classic` never throws for an
   ordinary, anticipated failure — a bad shape, an unmet precondition, a
   feature the caller asked for that the data doesn't have. Every such call
   returns a [`Status`](Status.md) (nothing to report except pass/fail) or a
   [`StatusOr<T>`](StatusOr.md) (pass/fail plus, on pass, the `T` you asked
   for). This is the single calling convention for the whole framework —
   `Flow::learn` returns a `Status`, `make_classifier` returns a
   `StatusOr<std::unique_ptr<ClassifierBase>>`, `crossval_plan` returns a
   `StatusOr<SplitPlan>`, and so on. Learn this pattern once here and every
   other page's `Status` / `StatusOr<...>` return type needs no re-explaining.

2. **The three option bags that travel with almost every call.**
   [`TrainOptions`](TrainOptions.md) configures a fit, [`InferOptions`](InferOptions.md)
   a prediction, [`AssessOptions`](AssessOptions.md) a scoring pass. They are
   independent of which estimator family you are using — a `Flow` wrapping a
   `KernelFamilyOptions` model and a bare `ClassifierBase` from
   `make_classifier(LinearFamilyOptions{...})` both take the exact same
   `TrainOptions`. [`RandomState`](RandomState.md) is the seed pair embedded
   inside `TrainOptions::random` and reused everywhere else a routine needs
   determinism (row balancing, tuning samplers, the isolation forest).

## Why `Status` Instead Of Exceptions

`classic` is meant to run inside larger pipelines — a `Flow` composed of a
dozen stages, a `tune` sweep evaluating hundreds of candidates — where one bad
row or one incompatible option should be a value the caller inspects and
routes, not a stack unwind that has to be caught at every call site up the
chain. Every call that can fail for a reason a caller might reasonably want to
handle (not a hard programming error) returns `Status` or `StatusOr<T>`
instead.

This does **not** mean `classic` never throws. A handful of paths still throw
for genuine programming errors — a contract violation so far from a valid call
that no `Status` code fits. But the public, documented contract of every
function in this reference is: check `.ok()` before trusting anything else
about the result.

## Reading Order

1. [`Status`](Status.md) / [`StatusOr`](StatusOr.md) — the calling convention,
   read this first regardless of which family you came here for.
2. [`TrainOptions`](TrainOptions.md) — read before [`estimators`](../estimators.md)
   or [`compose`](../compose.md); every `learn` takes one.
3. [`InferOptions`](InferOptions.md) / [`AssessOptions`](AssessOptions.md) —
   read alongside `infer` / `assess`.
4. [`RandomState`](RandomState.md) — a two-field struct, but worth knowing once
   since it recurs inside several other option bags.

## Related Pages

- [`data`](../data.md) — `FeatureBatch` / `LabeledBatch`, the values `learn` /
  `infer` / `assess` actually consume.
- [`estimators`](../estimators.md) — the `learn(batch, TrainOptions)` /
  `infer(batch, InferOptions)` / `assess(batch, AssessOptions)` contract.
