# Lymph `workflow`

`workflow` is lymph's answer to a Pipeline: the single object that carries a
raw-data admission contract, a branching preprocessing DSL, and a trainable
model together, so "run this on new data" means one call instead of a
hand-assembled sequence of steps a caller has to get right every time.

Three ideas, three types:

- **[`Policy`](workflow/Policy.md)** — a declarative contract for what a raw
  `DataFrame` is allowed to contain before modeling starts: which columns must
  be present, which must be dropped, which rows with missing values are
  rejected. Applied automatically to every raw frame a `Workflow` verb sees.
- **[`Payload`](workflow/Payload.md)** — a typed input carrier. Wraps
  whichever data shape you have (`DataFrame`, `FeatureBatch`, `LabeledBatch`)
  together with the roles that shape alone cannot express — label, sample
  weights, groups, a time key — so every `Workflow` verb takes one `Payload`
  argument instead of a growing parameter list.
- **[`Workflow`](workflow/Workflow.md)** — `Policy` + a branch/route DSL
  (route different raw columns through different prep chains, the way a
  `ColumnTransformer` would) + a [`models::Model`](models/Model.md). One
  object that takes raw data in and gives predictions out, with the same
  `learn` / `infer` / `assess` shape `Model` has, plus the policy and routing
  `Model` alone does not carry.

## What `workflow` Does Not Own

- **Fitting one model with no routing or admission contract** — that is
  [`models::Model`](models/Model.md) on its own; wrap it in a `Workflow` only
  when you need `Policy` or branch routing too.
- **Cross-validation, search, comparison** — [`selection`](selection.md) runs
  a `Workflow` (or a `Model`) under a validation plan; `workflow` itself never
  splits data or repeats a fit.
- **Metric computation** — [`metrics`](metrics.md) / [`reports`](reports.md).
  `Workflow::assess(...)` is a thin pass-through to the held `Model`'s own
  `assess`.
- **Production deployment hardening** (serving, versioning, rollout) — a
  future `procedure` family's concern, not this one's.

## Relationship To `models`

Every `Workflow` verb (`learn`, `map`, `infer`, `assess`, `save`) has the same
name and the same final behavior as the matching `Model` verb — `Workflow`
delegates to a held `Model` once `Policy` has been applied to any raw frame
argument. The difference is entirely in what happens *before* that: `Policy`
admission, and (optionally) the branch/route DSL building up the `Model`'s
prep stages column-by-column as you attach them, instead of you routing
columns yourself.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

workflow/Introduction
workflow/Workflow
workflow/Payload
workflow/Policy
workflow/TimeOptions
workflow/make
```
<!-- apiref:toctree:end -->
