# `pluss::ai::classic::io::save`

Write a [`Flow`](../compose/Flow.md)'s spec, fitted state (or a
training-batch fallback), and category maps to one self-contained file —
the producing half of the [`io`](../io.md) round trip. Two overloads: one
takes an explicit `train_batch` fallback, the other omits it (equivalent to
passing an empty `LabeledBatch{}`).

## Signature

```cpp
classic::Status classic::io::save(
    const std::string& path,
    const classic::Flow& flow,
    const classic::LabeledBatch& train_batch,
    const classic::TrainOptions& train_options = {},
    classic::io::FlowBundleMetadata metadata = {},
    const classic::CategoryMaps& category_maps = {});

classic::Status classic::io::save(
    const std::string& path,
    const classic::Flow& flow,
    const classic::TrainOptions& train_options = {},
    classic::io::FlowBundleMetadata metadata = {},
    const classic::CategoryMaps& category_maps = {});
```

The second overload is exactly the first with `train_batch = LabeledBatch{}`
— an empty, invalid batch that never serves as a fallback (see `train_batch`
below). Reach for the first overload whenever a fallback matters; the
second is for a `Flow` you are certain has a working fitted-state snapshot
and never needs to be re-fit on restore.

## What It Does

Serializes everything a `Flow` needs to be reconstructed later -- its spec
(model family, options, stages), its fitted state (or, failing that, a
training batch it can be re-fit from on restore), category maps for any
factorized columns, and free-form metadata -- into one file at `path`.
This is the write half of the round trip [`io::load`](load.md) reads back.

## Parameters

`path`
: The output file path, overwritten if it already exists.

`flow`
: The `Flow` to persist. Must have a model attached via
  [`set_model`](../compose/Flow/set_model.md) (checked via
  [`has_model_spec()`](../compose/Flow.md#state)) and must not contain any
  opaque stage added through
  [`append_transform`](../compose/Flow/append_transform.md) /
  [`add_function_transformer`](../compose/Flow/add_function_transformer.md) /
  [`add_feature_transformer`](../compose/Flow/add_feature_transformer.md)
  (checked via
  [`has_opaque_transforms()`](../compose/Flow.md#state)) — see `## Errors`
  for both rejections. `flow` need not be `trained()`; see `train_batch`
  below for what an untrained `flow` requires instead.

`train_batch` (first overload only)
: A fallback training batch, embedded in the bundle whenever a fitted-state
  snapshot is unavailable — see the table below for exactly when that is.
  An empty/invalid `train_batch` (one that fails
  [`validate_labeled`](../data/LabeledBatch.md), including the second
  overload's implicit `LabeledBatch{}`) simply means no fallback exists;
  `save` only fails over it when a fallback turns out to be required and
  none is usable:

  | `flow.trained()` | State-dump succeeds | `train_batch` valid | Outcome |
  | --- | --- | --- | --- |
  | `false` | (not attempted) | `true` | Saves via the batch fallback (`has_snapshot = false`) |
  | `false` | (not attempted) | `false` | `Status::invalid_argument` — nothing to save |
  | `true` | `true` | either | Saves the fitted-state snapshot (`has_snapshot = true`); `train_batch`, if valid, is embedded too but unused unless `restore` later needs it |
  | `true` | `false` | `true` | Saves via the batch fallback (`has_snapshot = false`) despite being trained |
  | `true` | `false` | `false` | `Status::invalid_argument` — nothing to save |

  "State-dump succeeds" means both
  [`dump_transform_states`](../compose/Flow/dump_transform_states.md) and
  [`dump_model_state`](../compose/Flow/dump_model_state.md) return `.ok()`
  — every family in this reference implements both, so the two
  `state-dump fails` rows are reachable in principle but not from any
  estimator or transform shipped here.

`train_options` (`TrainOptions`, default `{}`)
: Embedded in the bundle unconditionally (even when a fitted snapshot makes
  it unnecessary) — replayed by `restore`'s `learn` call whenever it falls
  back to `train_batch`.

`metadata` (`FlowBundleMetadata`, default `{}`)
: Written into the bundle largely as given — see
  [`FlowBundle`](FlowBundle.md#state) for every field. One field is
  special: `created_unix_nanos` left at its default `0` is silently
  replaced with the real current time before writing; a non-zero value you
  supply is kept exactly.

`category_maps` (`CategoryMaps`, default `{}`)
: Embedded in the bundle as given — see
  [`CategoryMaps`](../data/CategoryMap.md). Not validated against `flow` or
  `train_batch` in any way; passing the wrong maps (or none, for a `Flow`
  that needs them) saves successfully and only surfaces as a problem later,
  when reusing them to convert new raw data.

## Returns

A `Status`. `.ok()` once every byte has been written to `path`.

## Errors

- `!flow.has_model_spec()` — `Status::invalid_argument` (`"save requires a
  Flow configured via set_model(ModelSpec)."`), checked before anything
  else.
- `flow.has_opaque_transforms()` — `Status::invalid_argument` (`"save
  cannot serialize opaque transform stages added via
  append_transform()."`).
- No usable fitted-state snapshot and no valid `train_batch` — see the
  table above — `Status::invalid_argument` (`"save requires either
  fitted-state snapshot support or a valid training batch fallback."`).
- `path` cannot be opened for writing — `Status::invalid_argument`.
- A write failure partway through (disk full, permissions revoked
  mid-write) — `Status::internal`.

## Practical Examples

A trained `Flow` with a valid `train_batch` (the ordinary case), an
untrained `Flow` saved purely via the batch fallback (the second
overload), the same untrained `Flow` failing to save with no batch at all,
the two structural rejections (`has_model_spec()`, `has_opaque_transforms()`),
and the `created_unix_nanos` auto-fill.

### Example 1

A trained `Flow`, saved with its own training batch as a fallback — the
ordinary case. `train_batch` is present here but unused by `restore` (see
[`load`'s Example 1](load.md#example-1)), since the fitted-state snapshot
succeeds.

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

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_1.bundle", flow, batch);
  std::cout << "saved ok = " << std::boolalpha << saved.ok() << "\n";

  return 0;
}
```

Output

```text
saved ok = true
```

### Example 2

The second overload (no `train_batch` parameter) on an **untrained**
`Flow` still fails — there is no fitted state to snapshot and no fallback
batch. Compare with the first overload's own untrained-plus-batch case
below.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  flow.add_standard_scaler();
  // flow.learn(...) never called -- flow.trained() is false.

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_2.bundle", flow);
  std::cout << "saved ok = " << std::boolalpha << saved.ok() << "\n";
  std::cout << "message = \"" << saved.message << "\"\n";

  return 0;
}
```

Output

```text
saved ok = false
message = "save requires either fitted-state snapshot support or a valid training batch fallback."
```

### Example 3

The same untrained `Flow` as Example 2, but with a valid `train_batch`
supplied (the first overload) — saves successfully via the fallback path,
even though `flow.trained()` is `false`.

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
  // flow.learn(batch) never called -- flow.trained() is still false.

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_3.bundle", flow, batch);
  std::cout << "saved ok = " << std::boolalpha << saved.ok() << "\n";

  const cls::io::FlowBundle bundle = cls::io::load("/tmp/pluss_classic_io_save_example_3.bundle").value();
  std::cout << "bundle.trained      = " << bundle.trained << "\n";
  std::cout << "bundle.has_snapshot = " << bundle.has_snapshot << "\n";

  return 0;
}
```

Output

```text
saved ok = true
bundle.trained      = false
bundle.has_snapshot = false
```

### Example 4

`has_opaque_transforms()`: a `Flow` with a stage added via
`append_transform` cannot be saved at all, trained or not.

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
  flow.append_transform(std::make_unique<cls::prep::StandardScaler>());

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_4.bundle", flow, batch);
  std::cout << "saved ok = " << std::boolalpha << saved.ok() << "\n";
  std::cout << "message = \"" << saved.message << "\"\n";

  return 0;
}
```

Output

```text
saved ok = false
message = "save cannot serialize opaque transform stages added via append_transform()."
```

### Example 5

`has_model_spec()`: a transform-only `Flow` (no model attached) can
`learn()` successfully — but `save` still refuses it, since a bundle
always requires a `ModelSpec`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // empty -- no model attached
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series(
      {10.0, 12.0, 14.0, 16.0, 100.0, 102.0, 104.0, 106.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  const cls::Status learned = flow.learn(batch);
  std::cout << "learned ok = " << std::boolalpha << learned.ok() << "\n";

  const cls::Status saved = cls::io::save("/tmp/pluss_classic_io_save_example_5.bundle", flow, batch);
  std::cout << "saved ok = " << saved.ok() << "\n";
  std::cout << "message = \"" << saved.message << "\"\n";

  return 0;
}
```

Output

```text
learned ok = true
saved ok = false
message = "save requires a Flow configured via set_model(ModelSpec)."
```

### Example 6

`metadata.created_unix_nanos` left at its default `0` is replaced with the
real time before writing.

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

  cls::io::FlowBundleMetadata metadata{};  // created_unix_nanos left at 0
  cls::io::save("/tmp/pluss_classic_io_save_example_6.bundle", flow, batch, {}, metadata);

  const cls::io::FlowBundle bundle = cls::io::load("/tmp/pluss_classic_io_save_example_6.bundle").value();
  std::cout << "created_unix_nanos was auto-filled = " << std::boolalpha
            << (bundle.metadata.created_unix_nanos != 0) << "\n";

  return 0;
}
```

Output

```text
created_unix_nanos was auto-filled = true
```

## Changelog

- **0.1** — two overloads (explicit `train_batch`, implicit empty batch).

## See Also

- [`FlowBundle`](FlowBundle.md) — the field-by-field meaning of what gets
  written.
- [`load`](load.md) — reading the file back, including `restore`'s two
  fitted-state-versus-batch-fallback paths on read.
- [`Flow`](../compose/Flow.md) — `has_model_spec()` /
  `has_opaque_transforms()` / `dump_transform_states` / `dump_model_state`,
  every precondition and data source this function reads from `flow`.
