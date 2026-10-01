# `pluss::ai::classic::io::load` / `restore` / `load_and_restore`

Read a saved bundle back — `load` reads the file as plain data (a
[`FlowBundle`](FlowBundle.md)); `restore` turns an already-loaded
`FlowBundle` into a runnable [`Flow`](../compose/Flow.md); `load_and_restore`
does both in one call, the common case. All three share one on-disk format
and one schema-compatibility rule, so calling `load_and_restore` costs the
same as calling `load` then `restore` yourself.

## Signature

```cpp
classic::StatusOr<classic::io::FlowBundle> classic::io::load(
    const std::string& path);

classic::StatusOr<classic::Flow> classic::io::restore(
    const classic::io::FlowBundle& bundle);

classic::StatusOr<classic::Flow> classic::io::load_and_restore(
    const std::string& path);
```

## What It Does

Reads a previously saved bundle back off disk and turns it into a runnable
`Flow` again. `load` stops at the raw data (`FlowBundle`) without
constructing anything runnable; `restore` takes that data and rebuilds the
live `Flow` object (models, transforms, and all); `load_and_restore` is
both steps in one call -- the one to reach for unless the raw bundle needs
inspecting or editing before it becomes a `Flow`.

## Parameters

`path` (`load` / `load_and_restore`)
: The bundle file path to read.

`bundle` (`restore`)
: A [`FlowBundle`](FlowBundle.md) — ordinarily one `load` just returned,
  but nothing requires that; a hand-modified `FlowBundle` (built directly,
  or a `load`ed one with fields changed) is accepted the same way — see
  [Example 6](#example-6-restore-on-a-hand-modified-bundle).

## Returns

`load` returns a `StatusOr<FlowBundle>` — see [`FlowBundle`](FlowBundle.md)
for every field.

`restore` and `load_and_restore` return a `StatusOr<Flow>`, built one of
two ways:

- **Fitted-state path** — when `bundle.has_snapshot && bundle.trained`:
  [`load_transform_states`](../compose/Flow/load_transform_states.md) then
  [`load_model_state`](../compose/Flow/load_model_state.md) are called
  with the bundle's own saved state. If **both** succeed, the returned
  `Flow` is `trained()` immediately, with the exact parameters that were
  saved — no re-fitting.
- **Batch-fallback path** — taken whenever the fitted-state path is not
  attempted (`has_snapshot` or `trained` is `false`) **or** either
  `load_*_state` call above fails: `bundle.train_batch` is validated and
  [`Flow::learn`](../compose/Flow/learn.md) is called with it and
  `bundle.train_options`. A `Flow` built this way is fit fresh, not
  restored parameter-for-parameter — see [`save`'s parameter
  table](save.md#parameters) for which combination of "trained" /
  "state-dump succeeds" / "batch given" produced the bundle you are
  reading.

Either way, every stage from `bundle.transforms` is added to the returned
`Flow` first, via the same
[`add_named_transform`](../compose/Flow/add_stage.md) mechanism
[`Flow::add_stage`](../compose/Flow/add_stage.md) uses — this happens
before either path above runs, and its own failure (a corrupt or
incompatible `TransformSpec`) is returned immediately without attempting
either path.

## Errors

`load`:

- `path` cannot be opened for reading — `Status::invalid_argument`.
- The file's leading bytes do not match the expected magic value —
  `Status::invalid_argument` (`"load encountered invalid bundle magic."`).
- The bundle's stored schema version does not match the current reader's
  own version exactly — `Status::invalid_argument` (`"load schema version
  mismatch with current reader."`) — see
  [Example 5](#example-5-a-schema-version-mismatch). No older format is
  read tolerantly; a mismatch is always fatal, never a partial read.
- The file is truncated or corrupted partway through an otherwise-valid
  header — `Status::internal`, naming which section failed to read.
- The file is not a bundle at all (arbitrary content, so every
  length-prefixed field `load` reads off it is nonsense) —
  `Status::internal` (`"load encountered a corrupt or truncated
  bundle."`) — see [Example 4](#example-4). `load` reads every field
  through one guarded entry point specifically so a malformed file always
  comes back as this `Status` rather than as a crash, however garbled the
  bytes are.

`restore` / `load_and_restore`:

- `bundle.model` fails to build via
  [`make_flow`](../compose/make_flow.md) — that `Status`.
- A stage in `bundle.transforms` fails to attach — that `Status`.
- Both restore paths unavailable: the fitted-state path was not attempted
  or failed, **and** `bundle.train_batch` fails
  [`validate_labeled`](../data/LabeledBatch.md) — `Status::invalid_argument`
  (`"restore could not load fitted snapshots and bundle has no valid
  train-batch fallback."`).
- The batch-fallback path's own `Flow::learn` call fails — that `Status`.
- `load_and_restore` additionally fails with anything
  [`load`](#errors) itself can fail with, checked first.

## Practical Examples

A full fitted-state round trip via `load_and_restore` (predictions match
exactly, no re-fitting), the batch-fallback path taken because the saved
`Flow` was never trained, `load` and `restore` called as two separate
steps to show they compose to the same result, `load`'s rejection of a
non-bundle file, a schema-version mismatch, and `restore` called directly
on a hand-modified `FlowBundle` to force the fallback path.

### Example 1

A trained `Flow`, saved and restored — `infer` on the restored `Flow`
matches the original exactly, since the fitted-state path restores the
same parameters rather than re-fitting.

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

  const cls::Series original_scores = flow.infer(batch.features).value();

  const std::string path = "/tmp/pluss_classic_io_load_example_1.bundle";
  cls::io::save(path, flow, batch);

  const cls::Flow restored = cls::io::load_and_restore(path).move_value();
  const cls::Series restored_scores = restored.infer(batch.features).value();

  const double max_abs_diff =
      (original_scores - restored_scores).abs().amax(0, false).item<double>();
  std::cout << "restored.trained() = " << std::boolalpha << restored.trained() << "\n";
  std::cout << "max abs diff between predictions = " << max_abs_diff << "\n";

  return 0;
}
```

Output

```text
restored.trained() = true
max abs diff between predictions = 0
```

### Example 2

An untrained `Flow`, saved via the batch fallback (see [`save`'s Example
3](save.md#example-3)) — `load_and_restore` takes the batch-fallback path
and calls `learn` internally; the restored `Flow` is `trained()` even
though the one that was saved never was.

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
  // flow.learn(batch) never called.

  const std::string path = "/tmp/pluss_classic_io_load_example_2.bundle";
  cls::io::save(path, flow, batch);
  std::cout << "saved flow.trained() = " << std::boolalpha << flow.trained() << "\n";

  const cls::Flow restored = cls::io::load_and_restore(path).move_value();
  std::cout << "restored.trained()   = " << restored.trained() << "\n";
  std::cout << "restored infer ok    = " << restored.infer(batch.features).ok() << "\n";

  return 0;
}
```

Output

```text
saved flow.trained() = false
restored.trained()   = true
restored infer ok    = true
```

### Example 3

`load` and `restore` called as two separate steps produce the identical
`Flow` `load_and_restore` would have in one call — `load_and_restore` is
pure convenience, not a distinct code path.

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

  const std::string path = "/tmp/pluss_classic_io_load_example_3.bundle";
  cls::io::save(path, flow, batch);

  const cls::io::FlowBundle bundle = cls::io::load(path).value();
  const cls::Flow via_two_steps = cls::io::restore(bundle).move_value();
  const cls::Flow via_one_call = cls::io::load_and_restore(path).move_value();

  const cls::Series scores_a = via_two_steps.infer(batch.features).value();
  const cls::Series scores_b = via_one_call.infer(batch.features).value();
  const double max_abs_diff = (scores_a - scores_b).abs().amax(0, false).item<double>();
  std::cout << "max abs diff between the two paths = " << max_abs_diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between the two paths = 0
```

### Example 4

`load` on a file that is not a bundle at all — every length-prefixed field
`load` reads (starting with the magic bytes themselves) is validated
against what the file actually contains, so arbitrary content is always
rejected as a `Status`, never read past or trusted.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <fstream>
#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::string path = "/tmp/pluss_classic_io_load_example_4.bundle";
  std::ofstream not_a_bundle(path, std::ios::binary);
  not_a_bundle << "this file was never written by io::save";
  not_a_bundle.close();

  const cls::StatusOr<cls::io::FlowBundle> loaded = cls::io::load(path);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "load encountered a corrupt or truncated bundle."
```

### Example 5: A Schema-Version Mismatch

`save` writes whatever `metadata.schema_version` it is given, with no
check against the current reader's own version — only `load` enforces the
match. Saving with a deliberately wrong version makes the file
unreadable by this same build.

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

  cls::io::FlowBundleMetadata metadata{};
  metadata.schema_version = 999999;
  const std::string path = "/tmp/pluss_classic_io_load_example_5.bundle";
  cls::io::save(path, flow, batch, {}, metadata);

  const cls::StatusOr<cls::io::FlowBundle> loaded = cls::io::load(path);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "load schema version mismatch with current reader."
```

### Example 6: `restore` On A Hand-Modified Bundle

Loading a bundle that genuinely has a fitted-state snapshot, then forcing
`has_snapshot` off before calling `restore` directly — the batch-fallback
path runs instead, even though the original file had real fitted state to
offer.

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

  const std::string path = "/tmp/pluss_classic_io_load_example_6.bundle";
  cls::io::save(path, flow, batch);

  cls::io::FlowBundle bundle = cls::io::load(path).value();
  std::cout << "bundle.has_snapshot before edit = " << std::boolalpha << bundle.has_snapshot << "\n";
  bundle.has_snapshot = false;  // force the fallback path

  const cls::Flow restored = cls::io::restore(bundle).move_value();
  std::cout << "restored.trained() = " << restored.trained() << "\n";

  return 0;
}
```

Output

```text
bundle.has_snapshot before edit = true
restored.trained() = true
```

## Changelog

- **0.1** — `load`, `restore`, `load_and_restore`.

## See Also

- [`FlowBundle`](FlowBundle.md) — every field `load` produces and `restore`
  consumes.
- [`save`](save.md) — the producing function, including the table of which
  restore path a given bundle will take.
- [`Flow`](../compose/Flow.md) — `learn` / `infer` /
  `load_transform_states` / `load_model_state`, what `restore` calls.
