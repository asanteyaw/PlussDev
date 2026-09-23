# `classic::balance` Introduction

## What This Family Owns

`balance` owns two independent ways to respond to an imbalanced
classification target, plus the shared class-distribution summary both
read: **reweighting** (via
[`class_mix`](class_mix.md)/[`resolve_weight_plan`](resolve_weight_plan.md)
— every row counts, some just count more), and **rebalancing the rows
themselves** (via
[`plan_balance`](plan_balance.md)/[`balance_rows`](balance_rows.md) —
some rows are duplicated, synthesized, or removed so the data itself is
less skewed). These are not two ways of doing the same thing — a
downstream estimator can use either, both, or neither, and they compose
independently.

`resolve_weight_plan`/`resolve_training_weights` are exactly what
`TrainOptions::sample_weights`/`class_weight`/`class_weights` resolve to
internally before a fit; `balance_rows` is exactly what
`TrainOptions::balance` runs before a fit. You will rarely call any of
these four functions directly for an ordinary classifier fit — set the
matching `TrainOptions` fields instead, and the estimator calls them for
you. Reach for this family's functions directly when you want to inspect
the resolved weights or the rebalance plan yourself, or when rebalancing
data that is not about to go straight into one of this reference's own
estimators.

## Reweighting Vs. Rebalancing

| | Reweighting (`resolve_weight_plan`) | Rebalancing (`plan_balance`/`balance_rows`) |
| --- | --- | --- |
| What changes | Nothing about the rows — same data, same count | The actual rows: duplicated, synthesized, or removed |
| Sample count | Unchanged | Usually changes |
| Cost | Free — a per-row multiply | Real work: nearest-neighbor search for `BlendNeighbors`/cleaning |
| `TrainOptions` field | `sample_weights` / `class_weight` / `class_weights` | `balance` |
| Composes with the other | Yes — both can run on the same fit | Yes |

A `ClassWeightMode::Balanced` fit and a `BalanceTarget{kind =
Equalize}` fit can reach a similar practical effect (the model pays more
attention to the minority class either way), but they are not
interchangeable: reweighting never touches sample count or the actual
feature values a model sees, while rebalancing does — which matters for
any algorithm sensitive to sample count itself (a tree's split-finding
statistics, cross-validation fold sizes computed before this family ever
runs).

## `BlendNeighbors` Needs Real Feature Data — `ReuseRows` Does Not

[`plan_balance`](plan_balance.md) has two overloads for exactly this
reason: a plain `Series` of labels is enough to resolve target counts
and duplicate/drop indices (`growth = ReuseRows`, `clean = None`), but
`growth = BlendNeighbors` and any `clean` value both need real feature
values to find nearest neighbors — only the `LabeledBatch` overload
supports them, and the `Series`-only overload rejects both outright
rather than silently ignoring them. See
[`plan_balance`'s own parameter docs](plan_balance.md#parameters) for
the exact delegation rule between the two overloads.

## Cleaning Runs First, Against What's Left

When `BalanceTarget::clean != None`, cleaning always removes rows
**before** target counts are resolved — and the resolved target is
computed from the **post-cleaning** distribution, not the original one.
An `Equalize` target after cleaning removes rows from the majority class
targets the new, smaller majority count, not the one the data started
with. See [`plan_balance`'s Example 11](plan_balance.md#example-11) for
this demonstrated directly, and
[`BalanceTarget::protect_small_classes`](BalanceTarget.md#protect_small_classes)
for the one safeguard that keeps cleaning from working against the
imbalance it runs ahead of correcting.

## Reading Order

1. [`class_mix`](class_mix.md) / [`ClassMix`](ClassMix.md) — the shared
   class-distribution summary.
2. [`resolve_weight_plan`](resolve_weight_plan.md) — reweighting, the
   cheaper of the two responses.
3. [`BalanceTarget`](BalanceTarget.md) — every field governing
   rebalancing.
4. [`plan_balance`](plan_balance.md) / [`BalancePlan`](BalancePlan.md) —
   computing the rebalance recipe.
5. [`balance_rows`](balance_rows.md) — executing it.

## Related Pages

- [`core`](../core.md) — [`TrainOptions`](../core/TrainOptions.md),
  where both `resolve_weight_plan` and `balance_rows`'s logic reach
  every classifier automatically.
- [`data`](../data.md) — [`LabeledBatch`](../data/LabeledBatch.md), the
  input and output type for the `LabeledBatch`-taking functions here.
- [`neighbors`](../neighbors.md) — [`SpatialIndex`](../neighbors/SpatialIndex.md),
  the KD-tree `plan_balance` uses internally for `BlendNeighbors`
  synthesis and cleaning's own nearest-neighbor searches (on CPU).
