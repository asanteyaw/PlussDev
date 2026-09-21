# `pluss::ai::classic::prep`

`prep` owns the transform kernels: the concrete `TransformerBase`
implementations that turn raw feature columns into the numeric, fixed-width
form a model can consume. Every kernel here is a self-contained value you can
construct, `learn`, and `map` on its own — `prep` does not know about
pipelines, feature routing, or persistence of anything larger than one
transformer's own state.

Reach for [`compose`](compose.md) once you need several of these kernels
routed to different columns and fit together as one unit with a final model;
reach for [`inspection`](inspection.md) once you need to rank or select
features rather than reshape their values; reach for [`sparse`](sparse.md) for
the CSR-matrix-native counterparts used when the encoded width would make a
dense `FeatureBatch` wasteful.

## The Surface

`prep` is five sub-families, each owning one kind of reshaping:

| Sub-family | What it owns | First type to reach for |
| --- | --- | --- |
| [`scaling`](prep/scaling.md) | rescale numeric magnitudes without changing which value is largest | [`StandardScaler`](prep/scaling/StandardScaler.md) |
| [`encoding`](prep/encoding.md) | turn categorical values into numeric columns | [`OneHotEncoder`](prep/encoding/OneHotEncoder.md) |
| [`imputation`](prep/imputation.md) | fill in missing (`NaN`) values | [`SimpleImputer`](prep/imputation/SimpleImputer.md) |
| [`transform`](prep/transform.md) | reshape distributions, normalize vectors, or route custom logic | [`FeatureTransformer`](prep/transform/FeatureTransformer.md) |
| [`outlier`](prep/outlier.md) | score and flag anomalous rows | [`OutlierIsolator`](prep/outlier/OutlierIsolator.md) |

## How To Call It

Every kernel in `prep` is used the same three-call way, regardless of
sub-family — this is the one pattern worth internalizing before reading any
individual page:

```cpp
namespace cls = pluss::ai::classic;

cls::prep::StandardScalerOptions options{};
cls::prep::StandardScaler scaler(options);

scaler.learn(train_batch);                       // fit on training data
const pluss::Series scaled = scaler.map(train_batch).value();   // or on new data
// scaler.learn_map(train_batch) fuses the two calls above into one.
```

`learn` fits (statistics, category vocabularies, …); `map` applies the fitted
transform; `learn_map` does both against the same batch in one call. See
[`prep/Introduction`](prep/Introduction.md) for the full shared contract every
kernel here follows.

## Related Pages

- [`compose`](compose.md) — routing several `prep` kernels, plus a model,
  into one fit-once `Flow`.
- [`estimators`](estimators.md) — `TransformerBase`, the interface every
  kernel here implements.
- [`data`](data.md) — `FeatureBatch`, what every `learn`/`map` call consumes.
- [`sparse`](sparse.md) — sparse-native transforms for high-cardinality
  encoded output.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

prep/Introduction
prep/scaling
prep/encoding
prep/imputation
prep/transform
prep/outlier
```
<!-- apiref:toctree:end -->
