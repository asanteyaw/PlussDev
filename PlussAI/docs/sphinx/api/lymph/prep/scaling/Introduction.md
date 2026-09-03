# `prep::scaling` Introduction

## What This Subfamily Owns

`prep::scaling` owns four standalone numeric scaler objects —
[`prep::StandardScaler`](StandardScaler.md), `prep::MinMaxScaler`,
`prep::RobustScaler`, `prep::MaxAbsScaler` — and the option structs that
configure them ([`options::StandardScaler`](StandardScalerOptions.md) and its
siblings).

## Object, Not Stage

Every scaler here has a twin among the [`prep::stage`](../stage_family.md)
factories that does identical arithmetic from the identical option struct. The
subfamily exists for the cases where you want the scaler as a *value you hold*
rather than a step folded into a model:

- **Fit once, apply many times.** `learn()` on the training frame, then `map()`
  over every fold, every batch, or the inference frame — all against the same
  parameters.
- **Read the fitted parameters.** `summary()` exposes what was learned —
  `StandardScaler` names them (`mean`, `scale`); the others expose a
  `feature_names` list and a `state` tensor vector.
- **Compose by hand.** Drop a fitted scaler into a `prep::FeatureTransformer`
  branch, or into code that is not a `lymph` model at all.

If none of those apply, the [`prep::stage`](../stage_family.md) factory is the
shorter path.

## The Lifecycle In One Line

```cpp
lymph::prep::StandardScaler scaler;
const DataFrame scaled = scaler.features({"amount", "ratio"}).learn(train).map(later);
```

`features()` pins the columns (skip it to take every numeric column), `learn()`
fits and returns `*this` so it chains, `map()` replays. `learn_map(frame)` folds
the middle two together; `learned()` reports whether `learn` has run.

## Related Pages

- [`prep::stage`](../stage_family.md) — the factory twins; each stage page
  carries that transform's arithmetic and option toggles in full.
- [`prep::StandardScaler`](StandardScaler.md) — the worked-out exemplar of the
  shared method surface.
