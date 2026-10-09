<!-- pluss-hand-authored -->

# `lymph::io` — the load path

[Back to `io`](../io.md)

The read side of persistence. Three entry points, by how much you want back:

- [`models::Model::load`](../models/Model/load.md) — a fully trained `Model`.
- [`workflow::load`](../workflow/make.md) — a fully trained `Workflow`,
  `Policy` sidecar included.
- `lymph::io::load` — the raw `FlowBundle` value, to inspect (or to rebuild
  a `Workflow` with a `Policy` you choose via
  [`workflow::restore`](../workflow/make.md)).

## Signature

```cpp
// Fully-restored objects:
static models::Model models::Model::load(const std::string& path);
workflow::Workflow  workflow::load(const std::string& path);

// The raw bundle, plus the two ways to turn one into a Workflow:
lymph::StatusOr<lymph::io::FlowBundle> lymph::io::load(const std::string& path);
workflow::Workflow workflow::restore(const lymph::io::FlowBundle& bundle,
                                     workflow::Policy policy = {});
```

## What It Does

`lymph::io::load` reads a bundle file and returns it as a `FlowBundle` value without rebuilding anything. The model and workflow `load` functions are the usual entry points; this one is for inspecting a bundle, or for assembling a workflow with a policy of your choice. The result is a `StatusOr`, so an unreadable file is reported as a status.

## Parameters

`path`
: The bundle file written by a prior `save`. `workflow::load` also reads
  `path + ".policy"` if it exists; `Model::load` ignores any sidecar.

`bundle` (`workflow::restore`)
: A `FlowBundle` already read by `io::load`.

`policy` (`workflow::restore`)
: The admission contract for the rebuilt `Workflow` — **you supply it**.
  `restore` never reads a sidecar; that is `workflow::load`'s job.

## Returns

`Model::load`
: A trained `Model`, ready to `infer` / `assess` — no rebuild step.

`workflow::load`
: A trained `Workflow`; `.policy()` is whatever the sidecar held (an empty
  `Policy` if there was none).

`io::load`
: `StatusOr<FlowBundle>`. `FlowBundle` fields worth reading: `metadata`
  (`schema_version`, `producer`, `producer_version`), `trained`,
  `has_snapshot` (whether the training data was embedded), `transforms`
  (the prep-stage specs), `model` (the `ModelSpec`).

## Errors

- A `path` that does not exist, is not a bundle, or whose `schema_version`
  does not match the current one makes `io::load` return a non-`ok()`
  `StatusOr` — `Model::load` / `workflow::load` (which call `resolve` on it)
  throw.
- The schema is **not** read tolerantly: a bundle from a build with an older
  `schema_version` is rejected outright and must be re-saved.

## Practical Examples

### Example 1

`Model::load` — a static factory returning a trained model.

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

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_load_example_1.flow").string();
  {
    lymph::models::Model model = lymph::models::logistic_regression();
    model.learn_(x, y);
    model.save(path);
  }

  const lymph::models::Model loaded = lymph::models::Model::load(path);
  std::cout << "loaded.trained()   = " << std::boolalpha << loaded.trained() << "\n";
  std::cout << "loaded.task_kind() classification = "
            << (loaded.task_kind() == lymph::TaskKind::Classification) << "\n";
  std::cout << "pred = " << loaded.infer(x) << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
loaded.trained()   = true
loaded.task_kind() classification = true
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

`workflow::load` — the trained model plus its `Policy`, restored together.

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

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_load_example_2.flow").string();
  {
    workflow::Policy policy{};
    policy.drop({"customer_id"});
    workflow::make(lymph::models::logistic_regression(), policy)
        .learn(raw, data_options)
        .save(path);
  }

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained()                   = " << std::boolalpha << loaded.trained() << "\n";
  std::cout << "loaded.policy().drop_columns.size() = " << loaded.policy().drop_columns.size() << "\n";

  const DataFrame incoming({
      {"customer_id", Slate(pluss::make_series({9, 10, 11}, {3}, f64))},
      {"income", Slate(pluss::make_series({1.2, 4.2, 1.6}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.3, 2.2, 0.5}, {3}, f64))},
  });
  std::cout << "pred = " << loaded.infer(incoming, data_options) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");
  return 0;
}
```

Output

```text
loaded.trained()                   = true
loaded.policy().drop_columns.size() = 1
pred = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

### Example 3

`io::load` reads the `FlowBundle` for inspection; `workflow::restore` rebuilds
with a chosen `Policy`, not a sidecar one.

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

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_load_example_3.flow").string();
  {
    lymph::models::Model model = lymph::models::logistic_regression();
    model.prepare().scale({"income", "balance"});
    model.learn_(x, y);
    model.save(path);
  }

  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "schema_version   = " << bundle.metadata.schema_version << "\n";
  std::cout << "bundle.trained   = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "transform count  = " << bundle.transforms.size() << "\n";

  workflow::Policy policy{};
  policy.require({"income", "balance"});
  const workflow::Workflow restored = workflow::restore(bundle, policy);
  std::cout << "restored.trained() = " << restored.trained()
            << "  policy require count = " << restored.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
schema_version   = 5
bundle.trained   = true
transform count  = 1
restored.trained() = true  policy require count = 2
```

## See Also

- [`save`](save.md) — the write side.
- [`persistence`](persistence.md) — what round-trips and what does not.
- [`models::Model::load`](../models/Model/load.md) /
  [`workflow::make`](../workflow/make.md) — the full per-type reference.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
