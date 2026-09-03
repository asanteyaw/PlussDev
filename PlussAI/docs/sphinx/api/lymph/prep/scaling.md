# Lymph `prep::scaling`

`scaling` is the `prep` subfamily of **standalone, fittable numeric scalers** —
objects you construct, `learn()` on a frame, and then `map()` over as many later
frames as you like, with the fitted parameters held on the object and readable
through `summary()`.

Each scaler here is the object twin of a
[`prep::stage`](stage_family.md) factory: `prep::StandardScaler` does the same
arithmetic as `prep::stage::standard_scaler(...)`, from the same
[`options::StandardScaler`](scaling/StandardScalerOptions.md) struct. The
difference is ownership.

| Reach for… | when… |
| --- | --- |
| the [`prep::stage::*`](stage_family.md) factory | the scaling should travel *inside a model's* preparation plan and be fitted by `Model::learn` |
| the object here | the scaler is a *first-class thing you hold* — to fit once and reuse across frames, to inspect its learned parameters, or to wire into a `prep::FeatureTransformer` branch or non-model code |

## The scalers

| Object | Options | Transform |
| --- | --- | --- |
| [`prep::StandardScaler`](scaling/StandardScaler.md) | [`options::StandardScaler`](scaling/StandardScalerOptions.md) | `(x - mean) / std` |
| `prep::MinMaxScaler` | [`options::MinMaxScaler`](scaling/MinMaxScalerOptions.md) | rescale to a fixed `[lower, upper]` range |
| `prep::RobustScaler` | [`options::RobustScaler`](scaling/RobustScalerOptions.md) | `(x - median) / IQR` — outlier-resistant |
| `prep::MaxAbsScaler` | [`options::MaxAbsScaler`](scaling/MaxAbsScalerOptions.md) | `x / max(|x|)` — keeps sign and zero |

## The shared lifecycle

All four have the same method surface:

- [`features({...})`](scaling/StandardScaler.md#features) — pin the columns to
  act on. Optional: without it a scaler acts on every numeric column.
- [`learn(frame)`](scaling/StandardScaler.md#learn) — fit the parameters.
  Returns `*this`, so `scaler.features({...}).learn(frame)` chains.
- [`map(frame)`](scaling/StandardScaler.md#map) — apply the fitted parameters to
  a frame, returning the transformed copy.
- [`learn_map(frame)`](scaling/StandardScaler.md#learn_map) — `learn` then `map`
  in one call.
- [`learned()`](scaling/StandardScaler.md#learned) — has `learn` run yet?
- [`summary()`](scaling/StandardScaler.md#summary) — the fitted parameters.
  `StandardScaler` returns a `StandardScalerSummary` with named `mean` / `scale`
  fields; the other three return a `TransformerSummary` (`feature_names` plus a
  `state` tensor vector).

The arithmetic of each transform, and its option toggles, are covered on the
matching [`prep::stage`](stage_family.md) factory page — this subfamily is about
the *object* around it.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

scaling/Introduction
scaling/StandardScaler
scaling/MinMaxScaler
scaling/RobustScaler
scaling/MaxAbsScaler
```
<!-- apiref:toctree:end -->
