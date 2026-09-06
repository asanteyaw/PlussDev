# `workflow` Introduction

## What This Family Owns

`workflow` is the composition layer above [`models::Model`](../models/Model.md):
a `Workflow` is a `Model` plus everything that decides *what raw data reaches
that model* — a raw-frame admission contract, and (optionally) a
column-routing DSL for building up the model's prep pipeline branch by
branch. If a project only ever fits and infers with one model on data that
needs no admission checks and no per-column routing, `Model` alone is enough
and `Workflow` adds nothing; reach for `Workflow` the moment either of those
two concerns shows up.

Concretely:

- **Raw-frame admission** — [`Policy`](Policy.md): declared once,
  applied automatically before every `learn` / `map` / `infer` / `assess`
  call that receives a raw `DataFrame`. Required columns, forbidden columns,
  columns to silently drop, rows to reject on missing values.
- **Typed input** — [`Payload`](Payload.md): one value that can be a
  `DataFrame` with column-name roles, or an already-built `FeatureBatch` /
  `LabeledBatch`, plus label / weight / group / time roles attached
  separately from the data itself. Every `Workflow` verb has a `Payload`
  overload so callers do not have to know which concrete shape they are
  holding.
- **Column routing** — `Workflow::on(...)` / `Workflow::branch(...)`: route
  named columns (or columns matching a predicate) through their own chain of
  prep stages, mirroring a `ColumnTransformer`. Every routed stage lands on
  the held `Model`'s own prep pipeline — `Workflow` does not run a separate
  transform step of its own.
- **Time awareness** — [`TimeOptions`](TimeOptions.md): opts a `Workflow` (or
  a `Payload`) into the leakage-aware time contract that
  [`selection::validate::time`](../selection/validate/time.md) reads back out
  when building chronological folds.
- **Persistence** — `Workflow::save` / `workflow::load`: the held `Model`'s
  own `FlowBundle` save/load, plus a small sidecar file carrying `Policy` so
  a loaded `Workflow` re-applies the same admission contract a fresh one
  would.

## Compute Layer vs. Facade

`Workflow`'s six data verbs (`learn`, `map`, `learn_map`, `infer`,
`infer_scores`, `assess`) do not implement anything new — each one applies
`Policy` to any raw `DataFrame` argument, then calls the identically-named
verb on the held `Model`. Reach for `Model` directly when there is no policy
to enforce and no routing DSL in play; reach for `Workflow` the moment either
shows up, since retrofitting routing onto an already-built `Model` means
rebuilding it, while `Workflow::on(...)` records routed stages as you attach
them.

## Reading Order

1. [`Workflow`](Workflow.md) — the composition object itself: construction,
   the branch/route DSL (`on`), the six data verbs, `save`.
2. [`Payload`](Payload.md) — what every data verb actually accepts.
3. [`Policy`](Policy.md) — the admission contract `Workflow` enforces.
4. [`TimeOptions`](TimeOptions.md) — opting into time-aware validation.
5. [`make`](make.md) — every way to construct or reload a `Workflow`.

## Namespace

Everything here lives under `lymph::workflow` (`workflow::Workflow`,
`workflow::Payload`, `workflow::Policy`, `workflow::TimeOptions`). `Policy`'s
implementation home is technically `workflow::policy::Policy`, re-exported to
`workflow::Policy` — use the shorter spelling; the two name the same type.
`Payload`'s construction factory lives at `workflow::payload::make(...)`.

## Related Pages

- [`models::Model`](../models/Model.md) — the estimator every `Workflow`
  holds and delegates to.
- [`prep`](../prep.md) — the stages a routed branch attaches
  (`prep::stage::standard_scaler()` and the rest).
- [`selection::validate`](../selection/validate.md) — cross-validates a
  `Workflow` directly, `Payload` and all.
- [`options::Workflow`](../options.md) / [`options::Time`](../options.md) —
  the umbrella spellings of `WorkflowOptions` / `TimeOptions`.
