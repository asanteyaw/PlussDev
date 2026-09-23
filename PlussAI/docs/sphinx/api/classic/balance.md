# `pluss::ai::classic::balance`

`balance` owns two independent responses to an imbalanced classification
target: **reweighting** rows without changing them
([`class_mix`](balance/class_mix.md)/
[`resolve_weight_plan`](balance/resolve_weight_plan.md)) and
**rebalancing** the rows themselves — duplicating, synthesizing, or
removing them
([`plan_balance`](balance/plan_balance.md)/
[`balance_rows`](balance/balance_rows.md)). Both are exactly what
[`TrainOptions`](core/TrainOptions.md)'s own weighting fields and
`.balance` field resolve to internally before every classifier fit — see
[`Introduction`](balance/Introduction.md) for how the two compose and
where the family's one real algorithmic subtlety lives (cleaning runs
before, and against, the post-cleaning distribution).

## The Surface

| Type / function | What it gives you | Reach for it when |
| --- | --- | --- |
| [`class_mix`](balance/class_mix.md) / [`ClassMix`](balance/ClassMix.md) | class counts, proportions, inverse-frequency weights, imbalance ratio | you want the raw distribution, for either response below |
| [`resolve_weight_plan`](balance/resolve_weight_plan.md) | the final, mean-normalized per-row training weight | reweighting — no rows change, some just count more |
| [`BalanceTarget`](balance/BalanceTarget.md) | the desired class mix and how to reach it | configuring `plan_balance`/`balance_rows` |
| [`plan_balance`](balance/plan_balance.md) / [`BalancePlan`](balance/BalancePlan.md) | the resample/synthesize/clean recipe, without moving data | inspecting or reusing a rebalance plan before committing to it |
| [`balance_rows`](balance/balance_rows.md) | the rebalanced `LabeledBatch` directly | you just want the new batch |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::BalanceTarget target{};
target.kind = cls::BalanceTargetKind::Equalize;
target.growth = cls::BalanceGrowthKind::BlendNeighbors;

const cls::LabeledBatch balanced = cls::balance_rows(train_batch, target).value();
// every class in balanced.y now has the same row count as the original
// largest class, with extra rows synthesized from real neighbors.
```

## Related Pages

- [`core`](core.md) — [`TrainOptions`](core/TrainOptions.md), where this
  family's logic reaches every classifier automatically.
- [`data`](data.md) — [`LabeledBatch`](data/LabeledBatch.md), the input
  and output type.
- [`neighbors`](neighbors.md) — [`SpatialIndex`](neighbors/SpatialIndex.md),
  the KD-tree `plan_balance` uses for `BlendNeighbors` synthesis and
  cleaning.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

balance/Introduction
balance/class_mix
balance/resolve_weight_plan
balance/plan_balance
balance/balance_rows
balance/ClassMix
balance/BalancePlan
balance/BalanceTarget
```
<!-- apiref:toctree:end -->
