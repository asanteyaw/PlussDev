# Lymph `validate`

`validate` is the Lymph family for validation planning.

This is where you decide how evidence is earned.

In other words, the `validate` family does not define the model itself. It defines
the rules under which the model is allowed to prove itself:

- simple holdout
- repeated or unrepeated cross-validation
- grouped validation
- chronological validation
- last-block time holdout

That distinction matters because a good model trained under the wrong validation
policy is still a bad experiment.

## What This Family Owns

The `validate` family owns the public planning and execution surfaces that turn
validation intent into a reusable contract:

- `lymph::validate::Plan`
- `lymph::validate::cross(...)`
- `lymph::validate::time(...)`
- `lymph::validate::holdout(...)`

This family answers questions like:

- how should the data be partitioned?
- should order be preserved?
- should the final chronological block be pulled directly?
- should I keep the full fold report, only fold scores, or one summary score?

## What This Family Does Not Own

`validate` does not own the validation options bag.

That object is:

- [`lymph::metrics::ValidationOptions`](metrics/ValidationOptions.md)

The ownership split is intentional:

- `metrics::ValidationOptions` is the reusable public contract object
- `validate::*` is the family that consumes that contract to build executable
  validation plans

So:

- `metrics` owns the options object
- `validate` owns the planning surface

## Common Questions This Family Answers

Use this family when you need to answer questions like:

- how many folds should I run?
- should I shuffle or preserve order?
- do I need grouped evaluation?
- is this a time-aware workflow that must use chronological splits?
- do I want the full fold report, only the fold scores, or only the summary score?
- do I need the last chronological block directly as a holdout artifact?

## Main Objects

```{toctree}
:maxdepth: 2

split/Plan
split/cross
split/time
split/holdout
```

## Practical Reading Order

Read this branch in this order if you are learning it:

1. [ValidationOptions](metrics/ValidationOptions.md)
2. [Plan](split/Plan.md)
3. [cross](split/cross.md)
4. [time](split/time.md)
5. [holdout](split/holdout.md)

That order moves from public validation policy, to reusable plan, to specialized
entrypoints.
