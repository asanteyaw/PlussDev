<!-- pluss-hand-authored -->

# `lymph::workflow::Policy`

`Policy` is the raw-frame admission contract a [`Workflow`](Workflow.md)
applies before any prep stage or model sees data — a small, declarative,
replayable set of column rules (`drop` / `require` / `forbid`) and row rules
(`reject` / `allow`) that decide what is allowed in. It is governance, not
feature engineering: it does not scale, encode, or impute.

> See [Introduction](Policy/Introduction.md) for the column-rule / row-rule
> split and how it differs from preprocessing.

## Scope And Alias

`Policy` is `lymph::workflow::Policy` (implementation home
`lymph::workflow::policy::Policy`). There is no shorter `lymph::` alias —
write the full `workflow::` qualification, or bring in
`namespace workflow = pluss::ai::lymph::workflow;`.

## Construction

```cpp
lymph::workflow::Policy lymph::workflow::policy::make(lymph::workflow::Policy options = {});
```

`Policy` is an aggregate-friendly value type: `Policy{}` admits everything;
`Policy p; p.drop({"id"}).require({"income", "balance"});` chains.
`policy::make(options)` exists for symmetry with the other family factories —
it is an identity pass-through.

## State

`std::vector<std::string> drop_columns` · `require_columns` · `forbid_columns`
: The three column-rule lists — see [`columns`](Policy/columns.md).

`std::vector<std::string> reject_missing` · `allow_missing`
: The row-rule lists — see [`rows`](Policy/rows.md).

`bool filter_nulls` · `std::vector<std::string> null_filter_columns` · `bool drop_only_if_all_null`
: The legacy broad row-filter fields. Prefer `reject` /
  `reject_any_missing(...)` for new code — see [`rows`](Policy/rows.md).

`bool empty() const`
: `true` when every list is empty and `filter_nulls` is `false` — a genuine
  no-op `Policy`. See [`apply`](Policy/apply.md).

## Methods

| Group | Covers | Summary |
|-------|--------|---------|
| [`columns`](Policy/columns.md) | `drop`, `require`, `forbid` | The column rules — strip, demand, and forbid raw columns. |
| [`rows`](Policy/rows.md) | `reject`, `allow`, `reject_any_missing` | The row rules — which rows survive, by missing values. |
| [`apply`](Policy/apply.md) | `apply`, `empty` | Run the whole contract against a frame, in a fixed order. |

## Changelog

- **0.1** — `Policy` = `drop` / `require` / `forbid` / `reject_missing` /
  `allow_missing` + the legacy `filter_nulls` fields, and `apply(frame)`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Policy/Introduction
Policy/apply
Policy/columns
Policy/rows
```
<!-- apiref:toctree:end -->

## See Also

- [`Workflow`](Workflow.md) — holds a `Policy`, applies it on every
  raw-frame verb.
- [`Payload`](Payload.md) — a `Payload`'s frame also goes through `Policy`.
- [`Workflow::save`](Workflow/save.md) — persists a `Policy` as a sidecar
  file.
