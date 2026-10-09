<!-- pluss-hand-authored -->

# `lymph::io` — the save path

[Back to `io`](../io.md)

There is no bare `lymph::io::save` a caller reaches for — persistence is a
method on the artifact. This page is the cross-cutting reference for *writing*
one: [`models::Model::save`](../models/Model/save.md),
[`workflow::Workflow::save`](../workflow/Workflow/save.md), and what a bundle
file actually contains.

## Signature

```cpp
// Minimal: just the fitted state needed to reproduce predictions.
void models::Model::save(const std::string& path) const;
void workflow::Workflow::save(const std::string& path) const;

// Self-describing: also embed the training data + a provenance stamp.
void models::Model::save(
    const std::string& path,
    const pluss::table::DataFrame& x_train, const pluss::Series& y_train,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;
void workflow::Workflow::save(
    const std::string& path,
    const workflow::Payload& payload,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;

// Low-level: read a written bundle back as an inspectable value (see load).
lymph::StatusOr<lymph::io::FlowBundle> lymph::io::load(const std::string& path);
```

## What It Does

`save` writes a trained model or workflow to a bundle file. The minimal form writes only the fitted state needed to reproduce predictions. The self-describing form also embeds the training data and a provenance stamp, so the file can explain where the model came from. Saving does not change the object being saved.

## Parameters

`path`
: The bundle file to write. A single file; the containing directory must
  exist. `Workflow::save` writes a *second* file next to it, `path +
  ".policy"`, holding the [`Policy`](../workflow/Policy.md) sidecar.

`x_train` / `y_train` / `payload`
: The training inputs to embed. Supplying them switches on the
  self-describing form — the bundle then records not only *what* the model
  learned but *what from*. Bigger file; worth it for audit or re-fit later.

`train_options` (`lymph::options::Train`)
: The training options to record alongside the embedded data.

`metadata` (`lymph::io::FlowBundleMetadata`)
: A provenance stamp — `producer`, `producer_version`,
  `created_unix_nanos`, and the bundle `schema_version` (set for you). Read
  back with `io::load(path).metadata`.

## Returns

`Model::save` / `Workflow::save` return nothing; they write the file(s) and
throw on failure. `io::load` returns a `StatusOr<FlowBundle>` — the written
bundle as a value you can inspect without restoring a model
(see [the load page](load.md)).

## Errors

- `save` on an untrained `Model` / `Workflow` throws — there is no fitted
  state to write.
- An unwritable `path` (missing directory, permissions) throws.
- `Workflow::save(path, payload, ...)` throws the same weight-alignment
  error [`learn(payload)`](../workflow/Workflow/learn.md) does when the
  `Payload` carries `Series` weights without a named weight column.

## Practical Examples

### Example 1

`Model::save(path)` — the minimal write, and the round-trip that proves it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_1.flow").string();
  model.save(path);
  std::cout << "file written = " << std::boolalpha << std::filesystem::exists(path) << "\n";

  const lymph::models::Model loaded = lymph::models::Model::load(path);
  std::cout << "loaded.trained() = " << loaded.trained() << "\n";
  std::cout << "original = " << model.infer(x) << "\n";
  std::cout << "loaded   = " << loaded.infer(x) << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
file written = true
loaded.trained() = true
original = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
loaded   = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

`Workflow::save(path)` writes the bundle *and* the `.policy` sidecar;
`workflow::load` restores both.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_2.flow").string();
  trained.save(path);

  std::cout << "bundle written  = " << std::boolalpha << std::filesystem::exists(path) << "\n";
  std::cout << "sidecar written = " << std::filesystem::exists(path + ".policy") << "\n";

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.policy().drop_columns[0] = " << loaded.policy().drop_columns.front() << "\n";
  std::cout << "original = " << trained.infer(raw, data_options) << "\n";
  std::cout << "loaded   = " << loaded.infer(raw, data_options) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");
  return 0;
}
```

Output

```text
bundle written  = true
sidecar written = true
loaded.policy().drop_columns[0] = customer_id
original = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
loaded   = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 3

The self-describing form — embed a provenance stamp, read it back with
`io::load` without restoring the model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  lymph::io::FlowBundleMetadata metadata{};
  metadata.producer = "lymph_io_save_example_3";
  metadata.producer_version = "1.4.2";

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_3.flow").string();
  model.save(path, x, y, lymph::options::Train{}, metadata);

  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "metadata.producer         = " << bundle.metadata.producer << "\n";
  std::cout << "metadata.producer_version = " << bundle.metadata.producer_version << "\n";
  std::cout << "bundle.trained            = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "bundle.has_snapshot       = " << bundle.has_snapshot << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
metadata.producer         = lymph_io_save_example_3
metadata.producer_version = 1.4.2
bundle.trained            = true
bundle.has_snapshot       = true
```

## See Also

- [`load`](load.md) — the read side.
- [`persistence`](persistence.md) — the round-trip contract.
- [`models::Model::save`](../models/Model/save.md) /
  [`workflow::Workflow::save`](../workflow/Workflow/save.md) — the full
  per-type reference.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
