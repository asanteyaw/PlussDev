# `lymph::workflow::Policy` Introduction

## What This Type Represents

`Policy` is a raw-frame **admission contract** — a small, declarative,
replayable set of rules that decide *which columns may enter* a
[`Workflow`](../Workflow.md) and *which rows are admissible*, before any prep
stage or model sees the data. Every raw-frame `Workflow` verb runs
[`Policy::apply(frame)`](apply.md) first.

It is deliberately **governance, not feature engineering**. `Policy` does not
scale, encode, or impute — those are `Model` / prep-stage concerns, attached
through [`Workflow::on`](../Workflow/on.md). `Policy` only answers:

- these columns must be present (`require`)
- these columns must not be present (`forbid`)
- these columns are stripped before modeling (`drop`)
- rows missing a value in these columns are inadmissible (`reject`)

Keeping that separate from preprocessing is the point: admission rules stated
once on a `Policy` are replayed identically on every `learn` / `infer` /
`assess` / `save` call, instead of being ad hoc pre-fit and pre-infer code
that drifts out of sync between training and serving.

## Column Rules vs. Row Rules

The setters split cleanly into two groups:

- **[Column rules](columns.md)** — `drop`, `require`, `forbid`. They decide
  the *shape* of the admitted frame. `drop` never removes rows.
- **[Row rules](rows.md)** — `reject`, `allow`, `reject_any_missing`, and the
  legacy `filter_nulls` fields. They decide which *rows* survive, based on
  missing values.

[`apply`](apply.md) runs the whole contract in a fixed order: validate that
every named column exists, check `forbid`, then apply `drop`,
`reject_missing`, and the legacy sweep.

## Reading Order

1. [`columns`](columns.md) — `drop` / `require` / `forbid`.
2. [`rows`](rows.md) — `reject` / `allow` / `reject_any_missing`.
3. [`apply`](apply.md) — the method that runs it all, plus `empty()`.

## Namespace

`lymph::workflow::Policy` (implementation home
`lymph::workflow::policy::Policy`). No umbrella `lymph::` re-export — write
`workflow::Policy`, or bring in
`namespace workflow = pluss::ai::lymph::workflow;`. `Policy` is an
aggregate-friendly value type: `Policy{}` admits everything;
`Policy p; p.drop({...}).require({...});` chains.

## Related Pages

- [`Workflow`](../Workflow.md) — holds a `Policy`, applies it on every
  raw-frame verb.
- [`Workflow::save`](../Workflow/save.md) — persists the `Policy` as a
  sidecar file next to the model bundle.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="apply.html">
    <span class="label">Next</span>
    <span class="title">apply</span>
  </a>
</div>
<!-- apiref:nav:end -->
