# Lymph `io`

`io` is the cross-cutting family for **persistence** — writing a trained
artifact to disk and reading it back without retraining. It has no header of
its own; `lymph::io` is an alias for the underlying bundle read/write
primitives, and the user-facing save/load calls are methods on the artifact
types themselves.

## Where Persistence Actually Lives

| Call | Writes / reads | Full reference |
| --- | --- | --- |
| `models::Model::save(path)` / `Model::load(path)` | a trained model bundle | [`models/Model/save`](models/Model/save.md), [`models/Model/load`](models/Model/load.md) |
| `workflow::Workflow::save(path)` / `workflow::load(path)` | a model bundle **+ a `Policy` sidecar** (`path + ".policy"`) | [`workflow/Workflow/save`](workflow/Workflow/save.md), [`workflow/make`](workflow/make.md) |
| `lymph::io::load(path)` → `FlowBundle` | the raw bundle, as an inspectable value | [`io/load`](io/load.md) |
| `workflow::restore(bundle, policy)` | rebuild a `Workflow` from a bundle + a `Policy` you choose | [`workflow/make`](workflow/make.md) |

The `io/` pages below are the **cross-cutting** view — the write path, the
read path, and the round-trip contract in one place. Each links out to the
per-type reference for the exhaustive overload list.

## The Bundle

A saved file is a `FlowBundle`: `metadata` (a `schema_version` plus a
`producer` / `producer_version` provenance stamp), the `ModelSpec`, the
fitted prep-stage specs and their state, the estimator parameters,
category maps for `Note` columns, and — in the self-describing save form —
the training batch and its `TrainOptions`. The schema is versioned and read
strictly: a bundle from a build with a different `schema_version` is rejected
outright.

## Reading Order

1. [`save`](io/save.md) — the write path, minimal and self-describing forms.
2. [`load`](io/load.md) — `Model::load`, `workflow::load`, and the raw
   `io::load` → `FlowBundle`.
3. [`persistence`](io/persistence.md) — what round-trips, what does not, and
   Model-artifact vs. Workflow-artifact.

## Related Pages

- [`models`](models.md) / [`workflow`](workflow.md) — the types the
  save/load methods live on.
- [`options`](options.md) — the sibling umbrella family, for option structs
  rather than persistence.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

io/save
io/load
io/persistence
```
<!-- apiref:toctree:end -->
