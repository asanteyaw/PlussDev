# Lymph `prep::stage`

`stage` is the `prep` subfamily for **attachable transformation descriptors** —
the "what" half of a preprocessing step. It holds one small value type,
[`prep::Stage`](stage/Stage.md), and a set of factory functions that build one:
`prep::stage::standard_scaler(...)`, `prep::stage::one_hot_encoder(...)`, and the
rest.

## What a stage is, and is not

A `Stage` is a description: a transform kind plus that transform's options. It is
**not** a fitted object. Creating one touches no data, learns nothing, and
allocates almost nothing:

```cpp
const lymph::prep::Stage s = lymph::prep::stage::standard_scaler();
```

That value is inert until it is paired with a [`prep::Route`](route/Route.md) —
the "which columns" half — and handed to something that owns a preprocessing
plan:

- [`Model::append`](../models/Model/append.md) attaches the pair to a model, and
  the model fits it during `learn_`.
- [`Model::prepare`](../models/Model/prepare.md) is a fluent front end that emits
  the same stages under named shorthands.
- `prep::FeatureTransformer` attaches stages to individual routed branches.

The stage is resolved and fitted lazily, once a real frame exists, because
`append` is always called before any data is in hand.

## Stage factories vs. standalone transformers

Each stage factory has a twin in another `prep` subfamily — a standalone,
directly-fittable object with its own `.learn(frame)` / `.map(frame)`:

| Stage factory (this subfamily) | Standalone twin |
| --- | --- |
| `prep::stage::standard_scaler(opts)` | `prep::StandardScaler{opts}` ([`scaling`](scaling.md)) |
| `prep::stage::one_hot_encoder(opts)` | `prep::OneHotEncoder{opts}` ([`encoding`](encoding.md)) |
| `prep::stage::simple_imputer(opts)` | `prep::SimpleImputer{opts}` |
| ... | ... one per kind |

Both take the **same option struct**, so the arithmetic is identical; only the
return type and the ownership differ. Reach for a stage factory when the
transform should travel as part of a model's own contract. Reach for the
standalone object when the transform is a first-class thing you fit, inspect, and
reuse yourself. The two supervised encoders (`target_encoder`, `woe_encoder`) and
`custom` have no standalone twin — they are stage-only.

## The factory set

**Scaling** — column-wise rescaling:
[`standard_scaler`](stage/standard_scaler.md),
[`minmax_scaler`](stage/minmax_scaler.md),
[`robust_scaler`](stage/robust_scaler.md),
[`maxabs_scaler`](stage/maxabs_scaler.md).

**Encoding** — categorical columns to numbers:
[`one_hot_encoder`](stage/one_hot_encoder.md),
[`ordinal_encoder`](stage/ordinal_encoder.md),
[`label_encoder`](stage/label_encoder.md),
[`frequency_encoder`](stage/frequency_encoder.md),
[`basen_encoder`](stage/basen_encoder.md) (compact multi-column code),
[`hashing_encoder`](stage/hashing_encoder.md) (fixed width, no vocabulary), and
the two **supervised** encoders that read the training target —
[`target_encoder`](stage/target_encoder.md) (smoothed mean of target) and
[`woe_encoder`](stage/woe_encoder.md) (weight of evidence, binary target).

**Imputation** — filling gaps:
[`simple_imputer`](stage/simple_imputer.md) (one statistic),
[`neighbor_imputer`](stage/neighbor_imputer.md) (nearest complete rows),
[`iterative_imputer`](stage/iterative_imputer.md) (regression, iterated).

**Distribution and shape**:
[`quantile_map`](stage/quantile_map.md),
[`power_map`](stage/power_map.md),
[`threshold_binarizer`](stage/threshold_binarizer.md),
[`vector_normalizer`](stage/vector_normalizer.md) (row-wise),
[`polynomial_lift`](stage/polynomial_lift.md) (adds interaction terms).

**Arbitrary callables**: [`custom(name, fn)`](stage/custom.md) — wraps any
`Series -> Series` function as a stateless stage (attached with `Model::add`, not
`append`).

Every factory takes its transform's option struct with a default-constructed
value, so `prep::stage::standard_scaler()` and
`prep::stage::standard_scaler({.with_centering = false})` are both valid. The
supervised encoders are the exception to "inert until fitted": they still hold no
state at construction, but they require the target and so can only be fitted
inside [`Model::learn(x, y)`](../models/Model/learn.md), not a target-free
transform flow.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

stage/Introduction
stage/Stage
stage/standard_scaler
stage/minmax_scaler
stage/robust_scaler
stage/maxabs_scaler
stage/one_hot_encoder
stage/ordinal_encoder
stage/label_encoder
stage/frequency_encoder
stage/basen_encoder
stage/hashing_encoder
stage/target_encoder
stage/woe_encoder
stage/simple_imputer
stage/neighbor_imputer
stage/iterative_imputer
stage/quantile_map
stage/power_map
stage/threshold_binarizer
stage/vector_normalizer
stage/polynomial_lift
stage/custom
```
<!-- apiref:toctree:end -->
