<!-- pluss-hand-authored -->

# `lymph::io` — the persistence contract

[Back to `io`](../io.md)

What a saved bundle guarantees when it comes back, and where the line is.

## Signature

```cpp
// The four user-facing entry points the contract covers:
void          models::Model::save(const std::string& path) const;
static models::Model models::Model::load(const std::string& path);
void          workflow::Workflow::save(const std::string& path) const;
workflow::Workflow   workflow::load(const std::string& path);
```

## Parameters

`path`
: The bundle file. `save` writes it (and, for `Workflow`, a `path +
  ".policy"` sidecar); `load` reads it. The containing directory must exist.
  See [`save`](save.md) / [`load`](load.md) for the fuller overload set.

## The Round-Trip Promise

A saved artifact reloads into an object that **produces the same predictions
on the same input** — without retraining. Concretely, the bundle carries:

- the estimator's learned parameters
- every fitted prep stage's state (a scaler's centre/spread, an encoder's
  vocabulary, an imputer's fill values)
- the `ModelSpec` (algorithm + hyper-parameters)
- for a `Workflow`: the `Policy` (in the `.policy` sidecar) and the branch
  structure
- optionally, the training batch and `TrainOptions` (the self-describing
  form — see [`save`](save.md))
- category maps for any `DataNotePolicy::Factorize` column, so a raw `Note`
  column keeps encoding to the same integer codes

## What Does *Not* Round-Trip

- **Nothing that was not fitted.** Saving an untrained `Model` / `Workflow`
  throws.
- **Cross-version bundles.** The bundle `schema_version` must match the
  running build exactly; an older bundle is rejected, not read leniently.
  Re-save after a version bump.
- **Opaque custom stages.** A `Workflow` carrying a
  [`prep::stage::custom(...)`](../prep/stage/custom.md) function transformer
  cannot be cloned, and the value-returning `learn(...)` / `with(...)` throw
  on it — persistence of such a workflow is not supported.
- **Runtime context.** RNG state mid-stream, thread counts, device
  placement — a reload is a fresh process; only the modeling artifact is
  preserved.

## Model Artifact vs. Workflow Artifact

| Save this | When | The reload gives you |
| --- | --- | --- |
| `Model` | the fitted pipeline is the reusable unit | the learned estimator + prep stages |
| `Workflow` | the *deployable behavior* matters | the above **plus** the `Policy` admission contract |

If a `Policy` (dropped ID columns, forbidden leakage columns, row admission)
is part of how the model is meant to be used, the `Workflow` bundle is the
honest artifact — the `Model` bundle alone would lose that contract.

## Practical Examples

### Example 1

Fitted prep state round-trips: a scaler's learned parameters survive, so
`map()` output is identical after reload.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"a", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"b", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.prepare().scale({"a", "b"});
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_1.flow").string();
  model.save(path);
  const lymph::models::Model loaded = lymph::models::Model::load(path);

  std::cout << "original map =\n" << model.map(x) << "\n";
  std::cout << "loaded   map =\n" << loaded.map(x) << "\n";
  std::cout << "loaded.stage_count() = " << loaded.stage_count() << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
original map =
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-1.3416408, -1.3416408],
  [-0.4472136, -0.4472136],
  [0.4472136, 0.4472136],
  [1.3416408, 1.3416408]]
loaded   map =
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[-1.3416408, -1.3416408],
  [-0.4472136, -0.4472136],
  [0.4472136, 0.4472136],
  [1.3416408, 1.3416408]]
loaded.stage_count() = 1
```

### Example 2

The same trained model, saved as a `Model` and as a `Workflow`. Only the
`Workflow` bundle brings the `Policy` back.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  workflow::Policy policy{};
  policy.drop({"id"});

  const workflow::Workflow wf =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string model_path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_2_model.flow").string();
  const std::string wf_path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_2_wf.flow").string();

  wf.model().save(model_path);
  wf.save(wf_path);

  const lymph::models::Model loaded_model = lymph::models::Model::load(model_path);
  const workflow::Workflow loaded_wf = workflow::load(wf_path);

  std::cout << "loaded_model.trained()          = " << std::boolalpha << loaded_model.trained() << "\n";
  std::cout << "loaded_wf.trained()             = " << loaded_wf.trained() << "\n";
  std::cout << "loaded_wf.policy().drop_columns = " << loaded_wf.policy().drop_columns.size()
            << " (Model bundle has no Policy concept)\n";

  std::filesystem::remove(model_path);
  std::filesystem::remove(wf_path);
  std::filesystem::remove(wf_path + ".policy");
  return 0;
}
```

Output

```text
loaded_model.trained()          = true
loaded_wf.trained()             = true
loaded_wf.policy().drop_columns = 1 (Model bundle has no Policy concept)
```

### Example 3

The schema version is reported by `io::load`, and a bad path fails rather
than returning junk.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series a = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  const DataFrame x({{"a", Thread(a)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_3.flow").string();
  model.save(path);

  const lymph::io::FlowBundle ok = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "current schema_version = " << ok.metadata.schema_version << "\n";

  const std::string bogus =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_3_missing.flow").string();
  const auto result = lymph::io::load(bogus);
  std::cout << "load(missing).ok() = " << std::boolalpha << result.ok() << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
current schema_version = 5
load(missing).ok() = false
```

## See Also

- [`save`](save.md) / [`load`](load.md) — the write and read paths.
- [`models::Model::save`](../models/Model/save.md) — the bundle format in
  full.
- [`workflow::Workflow::save`](../workflow/Workflow/save.md) — the `Policy`
  sidecar.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
