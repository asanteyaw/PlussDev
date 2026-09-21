# `pluss::ai::classic::TransformerBase`

`TransformerBase` is the contract for objects that map features to features
rather than features to a label — [`make_transformer`](make_transformer.md)
returns one for any `ModelSpec`, regardless of `.task`. It does **not**
inherit [`EstimatorBase`](EstimatorBase.md); it is a separate interface with
its own shape: `learn` / `map` / `learn_map` instead of `learn` / `infer` /
`assess`, since there is no label to score a transform against.

## Signature

```cpp
virtual bool classic::TransformerBase::trained() const noexcept = 0;
virtual classic::Status classic::TransformerBase::learn(
    const classic::FeatureBatch&, const classic::TrainOptions& = {}) = 0;
virtual classic::StatusOr<pluss::Series> classic::TransformerBase::map(
    const classic::FeatureBatch&, const classic::InferOptions& = {}) const = 0;
virtual classic::StatusOr<pluss::Series> classic::TransformerBase::learn_map(
    const classic::FeatureBatch&,
    const classic::TrainOptions& = {},
    const classic::InferOptions& = {}) = 0;

// have a default body; a family may override:
virtual bool classic::TransformerBase::needs_target() const noexcept;  // default false
virtual classic::Status classic::TransformerBase::learn_supervised(
    const classic::FeatureBatch&, const pluss::Series& target, const classic::TrainOptions& = {});
virtual classic::StatusOr<std::vector<pluss::Series>> classic::TransformerBase::dump_state() const;
virtual classic::Status classic::TransformerBase::load_state(/* a range of pluss::Series */);
```

## Construction

Obtained only from [`make_transformer`](make_transformer.md). Held as
`std::unique_ptr<TransformerBase>`.

## State

`trained()` — as [`EstimatorBase::trained`](EstimatorBase/trained.md), but
declared separately here since `TransformerBase` does not inherit
`EstimatorBase`.

## Methods

`learn(batch, options = {})`
: Fits the transform (e.g. PCA's components, k-means' centroids) to `batch`.
  Returns a `Status`; `trained()` becomes `true` on success.

`map(batch, options = {})`
: Applies the already-fitted transform to `batch`, returning the mapped
  `Series`. `Status::precondition` if not yet trained.

`learn_map(batch, learn_options = {}, infer_options = {})`
: `learn` and `map` in one call, on the same data — fit and transform
  together rather than fitting once and mapping separately. The common case
  when you only need the fitted representation of the training data itself.

`needs_target()`
: `false` by default. `true` for a (target-aware) transform that requires a
  label during fitting. None of the nine `ModelFamily` transform paths
  reachable via `make_transformer` need one today — verified `false` for
  every family in this reference; a supervised prep stage that does need a
  target lives under `prep`, not here.

`learn_supervised(batch, target, options = {})`
: The target-aware fit path, called when `needs_target()` is `true`. The base
  default just forwards to `learn(batch, options)`, ignoring `target` — which
  is exactly what every family reachable via `make_transformer` does today,
  since none of them overrides `needs_target()` to `true`.

`dump_state()` / `load_state(state)`
: As [`EstimatorBase::dump_state`](EstimatorBase/dump_state.md) /
  [`load_state`](EstimatorBase/load_state.md) — same contract, same
  `Status::not_implemented` base default, same round-trip pattern.

## Practical Examples

### Example 1

`learn` then `map` as two separate calls, versus `learn_map` doing both at
once on the same data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 0.1, 0.2, 0.1, 10.0, 10.1, 9.9, 10.2}, {8}, f64).reshape({4, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> learn_then_map =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  learn_then_map.value()->learn(batch);
  const pluss::Series mapped = learn_then_map.value()->map(batch).value();

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> fused =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  const pluss::Series learn_mapped = fused.value()->learn_map(batch).value();

  std::cout << "learn() then map() = " << mapped << "\n";
  std::cout << "learn_map()        = " << learn_mapped << "\n";

  return 0;
}
```

Output

```text
learn() then map() = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [1],
  [0],
  [0]]
learn_map()        = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [1],
  [0],
  [0]]
```

### Example 2

`needs_target()` and `learn_supervised` — verified `false` / forwards to
`learn` for a `Cluster`-family transformer, since it does not override
either.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::ClusterFamilyOptions opts{};
  opts.clusters = 2;
  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, opts});
  std::cout << "needs_target() = " << std::boolalpha << made.value()->needs_target() << "\n";

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 0.1, 10.0, 10.1}, {4}, f64).reshape({4, 1});
  const pluss::Series unused_target = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::Status learned = made.value()->learn_supervised(batch, unused_target);
  std::cout << "learn_supervised ok = " << learned.ok() << "\n";
  std::cout << "trained()           = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
needs_target() = false
learn_supervised ok = true
trained()           = true
```

### Example 3

`map` before `learn` fails, the same way `EstimatorBase::infer` does.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Decompose,
                                           cls::DecomposeFamilyOptions{}});

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({1, 2});
  const cls::StatusOr<pluss::Series> mapped = made.value()->map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "PcaTransformer requires learn() before map()."
```

## Changelog

- **0.1** — `learn` / `map` / `learn_map`, `trained`, `needs_target` +
  `learn_supervised`, `dump_state` / `load_state`.

## See Also

- [`EstimatorBase`](EstimatorBase.md) — the parallel contract for
  classifiers/regressors; not a base of this type.
- [`make_transformer`](make_transformer.md) — how to obtain one.
- [`ClusterFamilyOptions`](../families/ClusterFamilyOptions.md) /
  [`DecomposeFamilyOptions`](../families/DecomposeFamilyOptions.md) /
  [`DetectFamilyOptions`](../families/DetectFamilyOptions.md) /
  [`KernelFamilyOptions`](../families/KernelFamilyOptions.md) (one-class
  form) — the four families [`make_transformer`](make_transformer.md)
  actually supports.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
