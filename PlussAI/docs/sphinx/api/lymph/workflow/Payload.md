<!-- pluss-hand-authored -->

# `lymph::workflow::Payload`

`Payload` is the typed data-and-role carrier every [`Workflow`](Workflow.md)
data verb accepts: a `DataFrame` / `FeatureBatch` / `LabeledBatch` plus the
roles attached to it — features, label, weights, groups, time — as one value
instead of a parameter list that grows with every new role.

> See [Introduction](Payload/Introduction.md) for why it exists, the three
> starting shapes, and the reading order.

## Scope And Alias

`Payload` is `lymph::workflow::Payload` (implementation home
`lymph::workflow::payload::Payload`). There is no shorter `lymph::` alias —
write the full `workflow::` qualification, or bring in
`namespace workflow = pluss::ai::lymph::workflow;`.

## Construction

```cpp
lymph::workflow::Payload lymph::workflow::payload::make(
    pluss::table::DataFrame frame, lymph::DataFrameBatchOptions options = {});
lymph::workflow::Payload lymph::workflow::payload::make(lymph::FeatureBatch features);
lymph::workflow::Payload lymph::workflow::payload::make(lymph::LabeledBatch labeled);
```

`make(frame)` is the common entry point; the role setters
([`label`](Payload/label.md), [`weights`](Payload/weights.md),
[`time`](Payload/time.md), and the rest on [`roles`](Payload/roles.md)) fill
in `options` afterward. `make(FeatureBatch)` / `make(LabeledBatch)` wrap data
the caller already assembled.

## State

`bool has_frame() const` · `has_features() const` · `has_labeled() const`
: Which of the three constructed shapes this `Payload` holds.

`bool has_label() const` · `has_label_feature() const`
: Whether supervision was attached by `Series` vs. by column name — see
  [`label`](Payload/label.md). Mutually exclusive.

`bool has_weights() const` · `has_weight_feature() const` · `has_external_weights() const`
: Whether weights are attached and by which route — see
  [`weights`](Payload/weights.md).

`bool has_groups() const` · `has_time() const`
: Whether [`groups(...)`](Payload/roles.md) / any
  [`time(...)`](Payload/time.md) overload was called.

`const DataFrameBatchOptions& data() const` / `DataFrameBatchOptions& data()`
: The options record the setters write into. Mutable access reaches a field
  with no dedicated setter.

`const pluss::table::DataFrame& frame() const` · `const FeatureBatch& features() const` · `const LabeledBatch& labeled() const`
: The held data; each throws if the `Payload` was not built from that shape.

`const Series& label() const` · `weights() const` · `groups() const` · `time() const`
: The externally-supplied `Series` for each role — `Series::undefined()` if
  that role was set by name, or not at all.

`std::string time_feature() const` · `const std::optional<TimeOptions>& time_options() const`
: The time column's name, and the full options struct when
  [`time(TimeOptions)`](Payload/time.md) was used.

## Methods

| Group | Covers | Summary |
|-------|--------|---------|
| [`label`](Payload/label.md) | `label(name)`, `label(Series)` | Attach the supervised target, by column name or by an external vector. |
| [`weights`](Payload/weights.md) | `weights(name)`, `weights(Series)` | Attach per-row training weights, plus the frame-backed alignment rule. |
| [`time`](Payload/time.md) | `time(name)`, `time(TimeOptions)`, `time(Series)` | Attach the time role, from a name, a full contract, or a vector. |
| [`roles`](Payload/roles.md) | `features`, `groups`, `note`, `layout`, `thread` | The remaining role and data-conversion-policy setters. |
| [`batch`](Payload/batch.md) | `feature_batch`, `labeled_batch`, `with_frame` | Materialize the `Payload` into the batch every verb consumes. |

## Changelog

- **0.1** — `Payload` = frame/features/labeled union + label / weights /
  groups / time role attachment + `feature_batch()` / `labeled_batch()`
  reduction.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Payload/Introduction
Payload/batch
Payload/label
Payload/roles
Payload/time
Payload/weights
```
<!-- apiref:toctree:end -->

## See Also

- [`Workflow`](Workflow.md) — every data verb accepts a `Payload`.
- [`Policy`](Policy.md) — a `Payload`'s frame goes through `Policy` on every
  raw-frame verb.
- [`TimeOptions`](TimeOptions.md) — the richer form of `time(...)`.
