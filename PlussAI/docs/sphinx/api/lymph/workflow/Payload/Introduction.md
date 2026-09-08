# `lymph::workflow::Payload` Introduction

## What This Type Represents

`Payload` is a single value that carries a dataset *and* the roles attached
to it — which columns are features, which one is the label, where the sample
weights are, which column is the grouping key, which column is the time
index. Every data verb on [`Workflow`](../Workflow.md) —
[`learn`](../Workflow/learn.md), [`map`](../Workflow/map.md),
[`infer`](../Workflow/infer.md), [`assess`](../Workflow/assess.md),
[`save`](../Workflow/save.md) — has a `Payload` overload alongside its
`(frame, DataFrameBatchOptions)` and bare-batch forms.

It exists so a call that needs more than "features and a label" does not turn
into a parameter list that grows every time a new role appears. Instead of

```cpp
workflow.learn(frame, feature_names, label_col, weight_col, group_col, time_col, note_policy);
```

you attach the roles once, by name, on the `Payload`:

```cpp
auto payload = workflow::payload::make(frame)
                   .features({"income", "balance"})
                   .label("default")
                   .weights("row_weight")
                   .time("observed_at");
workflow.learn(payload);
```

`Payload` is a plain, cheap-to-copy value — build one, hand it to as many
verbs as you like.

## The Three Starting Shapes

`workflow::payload::make(...)` has three overloads, one per shape the caller
already holds:

- **`make(DataFrame frame, DataFrameBatchOptions options = {})`** — a raw
  table. This is the common case; the role setters below fill in `options`.
- **`make(FeatureBatch features)`** — an already-assembled feature tensor,
  no label. For inference-only or unsupervised use.
- **`make(LabeledBatch labeled)`** — an already-assembled features + label
  (+ optional weights) bundle. The role setters are mostly moot on this
  form, since the batch already carries everything.

Which shape a `Payload` holds is what `has_frame()` / `has_features()` /
`has_labeled()` report.

## Reading Order

1. [`label`](label.md) — supervision, by column name or by an external
   `Series`.
2. [`weights`](weights.md) — per-row training weights, and the alignment
   rule for frame-backed payloads.
3. [`time`](time.md) — the time role, from a name, a full
   [`TimeOptions`](../TimeOptions.md), or a `Series`.
4. [`roles`](roles.md) — `features(...)`, `groups(...)`, and the
   `note`/`layout`/`thread` data-conversion shortcuts.
5. [`batch`](batch.md) — `feature_batch()` / `labeled_batch()` /
   `with_frame()`, the materialization step every verb runs internally.

## Namespace

`lymph::workflow::Payload` (implementation home
`lymph::workflow::payload::Payload`). No umbrella `lymph::` re-export — write
`workflow::Payload`, or bring in
`namespace workflow = pluss::ai::lymph::workflow;`.

## Related Pages

- [`Workflow`](../Workflow.md) — every data verb accepts a `Payload`.
- [`Policy`](../Policy.md) — a `Payload`'s frame goes through `Policy` on
  every raw-frame verb, exactly as a bare frame argument would.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="batch.html">
    <span class="label">Next</span>
    <span class="title">batch</span>
  </a>
</div>
<!-- apiref:nav:end -->
