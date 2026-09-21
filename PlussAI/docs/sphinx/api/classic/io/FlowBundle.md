# `pluss::ai::classic::io::FlowBundle`

Everything a saved `Flow` file contains, read back as plain data — the
[`ModelSpec`](../families/ModelSpec.md), every
[`TransformSpec`](../compose/TransformSpec.md), fitted state (when the file
has one), a training-batch fallback (when it has one instead or as well),
and the category maps a raw-`Note`-fitted `Flow` needs. Produced by
[`load`](load.md); consumed by `restore` to build an actual `Flow`. Reading
a `FlowBundle` directly — without calling `restore` — is how you inspect a
saved file's spec or metadata without paying to rebuild or re-fit anything.

## Signature

```cpp
classic::io::FlowBundleMetadata classic::io::FlowBundle::metadata;
classic::ModelSpec classic::io::FlowBundle::model;
std::vector<classic::TransformSpec> classic::io::FlowBundle::transforms;
bool classic::io::FlowBundle::trained;
bool classic::io::FlowBundle::has_snapshot;
std::vector<std::vector<pluss::Series>> classic::io::FlowBundle::transform_states;
std::vector<pluss::Series> classic::io::FlowBundle::model_state;
classic::TrainOptions classic::io::FlowBundle::train_options;
classic::LabeledBatch classic::io::FlowBundle::train_batch;
classic::CategoryMaps classic::io::FlowBundle::category_maps;

// classic::io::FlowBundle::metadata's type:
std::uint32_t classic::io::FlowBundleMetadata::schema_version;
std::string classic::io::FlowBundleMetadata::producer;
std::string classic::io::FlowBundleMetadata::producer_version;
std::uint64_t classic::io::FlowBundleMetadata::created_unix_nanos;
```

## Construction

Produced only by [`load`](load.md). A plain value type — copy or store it
freely. Building one by hand and handing it to `restore` yourself is legal
(nothing checks that a `FlowBundle` you construct actually came from
`load`) but unusual — the field-by-field contract below is written for
"read back what `load` gave you," not for hand assembly.

## State

`metadata` — `FlowBundleMetadata`
: Provenance, not part of the model itself:

  `schema_version` — `std::uint32_t`
  : The bundle format version the file was written with. `load` already
    checked this equals the current reader's own version before returning
    (see [`load`'s Errors](load.md#errors)) — by the time you hold a
    `FlowBundle`, this field is guaranteed to match, and reading it back
    is mostly a formality.

  `producer` / `producer_version` — `std::string`
  : Which library and version wrote the file. Default `"pluss.ai.classic"`
    / `"0.1.0"` when `save` was called with a default-constructed
    `FlowBundleMetadata` — informational only, never checked by `load`.

  `created_unix_nanos` — `std::uint64_t`
  : Nanoseconds since the Unix epoch when `save` wrote the file. `save`
    fills this in for you when the `FlowBundleMetadata` you pass it leaves
    it at the default `0` — see [`save`](save.md#parameters).

`model` — `ModelSpec`
: The spec `restore` rebuilds via [`make_flow`](../compose/make_flow.md).
  Readable directly without calling `restore` — check `.task` / `.family`
  before committing to a full restore.

`transforms` — `std::vector<TransformSpec>`
: Every stage, in the same order `restore` replays them via
  [`Flow::add_named_transform`](../compose/Flow/add_stage.md). Always
  present regardless of `trained`/`has_snapshot` — a bundle from an
  unfitted `Flow` still carries its stage configuration.

`trained` — `bool`
: Whether the source `Flow` was `trained()` at save time.

`has_snapshot` — `bool`
: Whether `transform_states` / `model_state` hold real fitted parameters.
  `false` means the bundle relies entirely on `train_batch` /
  `train_options` instead — see
  [`Introduction`](Introduction.md#two-ways-a-bundle-restores-a-fitted-flow)
  for exactly when each path is taken.

`transform_states` / `model_state` — `std::vector<std::vector<pluss::Series>>` / `std::vector<pluss::Series>`
: The raw output of [`dump_transform_states`](../compose/Flow/dump_transform_states.md) /
  [`dump_model_state`](../compose/Flow/dump_model_state.md) at save time —
  empty when `has_snapshot` is `false`. `restore` feeds these straight back
  into [`load_transform_states`](../compose/Flow/load_transform_states.md) /
  [`load_model_state`](../compose/Flow/load_model_state.md).

`train_options` — `TrainOptions`
: Always present (even when `has_snapshot` is `true` and it goes unused) —
  the options `save` was called with, replayed by `restore`'s `learn` call
  when it falls back to `train_batch`.

`train_batch` — `LabeledBatch`
: The training data fallback — a default, empty `LabeledBatch{}` when
  `save` was not given one (or was given one that failed
  [`validate_labeled`](../data/LabeledBatch.md)). Present alongside a real
  `has_snapshot = true` too, whenever `save` was given both a fitted `Flow`
  and a `train_batch` — `restore` only reaches it if the fitted-snapshot
  path fails.

`category_maps` — `CategoryMaps`
: The fitted [`CategoryMaps`](../data/CategoryMap.md) `save` was given —
  empty when the source `Flow` was never fit from raw `Note` columns via
  `Factorize`. `restore` does not consume this itself (a restored `Flow`
  has no notion of category maps); it is carried through for you to reuse
  when converting new raw data the same way the original training batch
  was converted.

## Methods

None — `FlowBundle` is a plain data holder, read directly.

## Practical Example

A round trip stopping short of `restore`: `load` a saved bundle and read
its metadata, spec, and snapshot flag directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  const std::string path = "/tmp/pluss_classic_io_flowbundle_example_1.bundle";
  cls::io::save(path, flow, batch);

  const cls::io::FlowBundle bundle = cls::io::load(path).value();
  std::cout << "producer       = " << bundle.metadata.producer << "\n";
  std::cout << "schema_version = " << bundle.metadata.schema_version << "\n";
  std::cout << "model.family   = " << static_cast<int>(bundle.model.family) << "\n";
  std::cout << "trained        = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "has_snapshot   = " << bundle.has_snapshot << "\n";
  std::cout << "transforms.size() = " << bundle.transforms.size() << "\n";

  return 0;
}
```

Output

```text
producer       = pluss.ai.classic
schema_version = 5
model.family   = 0
trained        = true
has_snapshot   = true
transforms.size() = 1
```

## Changelog

- **0.1** — `metadata` (a `FlowBundleMetadata`), `model`, `transforms`,
  `trained`, `has_snapshot`, `transform_states`, `model_state`,
  `train_options`, `train_batch`, `category_maps`.

## See Also

- [`save`](save.md) / [`load`](load.md) — the producing/consuming
  functions.
- [`Flow`](../compose/Flow.md) — the type a `FlowBundle` is saved from and
  restored back into.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
