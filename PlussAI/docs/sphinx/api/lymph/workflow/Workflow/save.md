<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e52c5954be9d -->

# `lymph::workflow::Workflow::save`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
void Workflow::save(const std::string& path) const;
void Workflow::save(const std::string& path, io::FlowBundleMetadata metadata) const;
void Workflow::save(
    const std::string& path,
    const pluss::table::DataFrame& raw_train_frame,
    const DataFrameBatchOptions& data_options,
    const TrainOptions& train_options = {},
    io::FlowBundleMetadata metadata = {}) const;
void Workflow::save(
    const std::string& path,
    const Payload& payload,
    const TrainOptions& train_options = {},
    io::FlowBundleMetadata metadata = {}) const;

// Free functions -- the Policy sidecar this method writes/reads, exposed
// standalone for a Policy that needs to travel independently of a bundle:
void workflow::save_policy(const std::string& bundle_path, const Policy& policy);
Policy workflow::load_policy(const std::string& bundle_path);
Workflow workflow::load(const std::string& path);
```

## What `Workflow` Adds

`save` writes the held `Model`'s trained bundle exactly as
[`models::Model::save`](../../models/Model/save.md) would, then writes this
`Workflow`'s `Policy` to a sidecar file at `path + ".policy"` — one extra
call, `workflow::save_policy(path, policy_)`, after the model bundle is
written. [`workflow::load`](../make.md) reads both back and reconstructs a
`Workflow` that applies the identical `Policy` on every subsequent call
(Example 1). `save` requires a **trained** `Workflow` — saving a template is
an error, same as `Model::save`.

The `(path, raw_train_frame, data_options, train_options, metadata)` overload
embeds the training data into the bundle for a self-describing archive — see
[`Model::save`'s "What It Does"](../../models/Model/save.md#what-it-does) for
why. The frame is run through `Policy::apply(...)` first, so the embedded
data matches what the model actually trained on, not the pre-`Policy` raw
input. The `(path, payload, train_options, metadata)` overload is the
`Workflow`-only equivalent for a `Payload`-supplied training set (Example 2).

## Input Shapes

`path` alone, or `path` plus a `FlowBundleMetadata` stamp, write the minimal
bundle. `(path, raw_train_frame, data_options, train_options, metadata)` and
`(path, payload, train_options, metadata)` additionally embed the training
data — see [`Model::save`'s Parameters](../../models/Model/save.md#parameters)
for the full field list on `metadata` and `train_options`.

## Returns

Nothing — `save` writes two files (the bundle at `path`, the `Policy`
sidecar at `path + ".policy"`) and returns.

## Errors

- Every error `Model::save` itself can raise (calling `save` on an untrained
  `Workflow`, an unwritable `path`) applies unchanged.
- `save(path, payload, ...)` throws the same weight-alignment error
  [`learn(payload)`](learn.md#errors) does, for the same reason, when the
  `Payload` carries `Series` weights without a named weight column.

## Practical Examples

### Example 1

Baseline round-trip: `save`/[`load`](../make.md) restore both the trained
`Model` and this `Workflow`'s `Policy` — a reloaded `Workflow` predicts
identically and still drops the same raw column.

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
      {"customer_id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_1.bundle").string();
  trained.save(path);

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.policy().drop_columns == original: "
            << std::boolalpha << (loaded.policy().drop_columns == policy.drop_columns) << "\n";

  const pluss::Series original_pred = trained.infer(raw, data_options);
  const pluss::Series loaded_pred = loaded.infer(raw, data_options);
  std::cout << "original_pred = " << original_pred << "\n";
  std::cout << "loaded_pred   = " << loaded_pred << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
```

Output

```text
loaded.policy().drop_columns == original: true
original_pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
loaded_pred   = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

`save(path, payload, train_options, metadata)` embeds the `Payload`'s
training data (post-`Policy`) into the bundle alongside the fitted model.

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
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  const workflow::Payload payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);
  const workflow::Workflow trained = workflow::make(lymph::models::logistic_regression()).learn(payload);

  lymph::io::FlowBundleMetadata metadata{};
  metadata.producer = "lymph_workflow_save_example_2";
  metadata.producer_version = "0.1";

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_2.bundle").string();
  trained.save(path, payload, lymph::options::Train{}, metadata);

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained() = " << std::boolalpha << loaded.trained() << "\n";
  std::cout << "pred = " << loaded.infer(payload) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
```

Output

```text
loaded.trained() = true
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 3

`save_policy`/`load_policy` directly, standalone — the same round-trip
`save`/`load` do internally, useful when a `Policy` needs to travel
independently of any particular saved model bundle.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  workflow::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance"});
  policy.reject_any_missing({"income"});

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_3.policy_only").string();
  workflow::save_policy(path, policy);

  const workflow::Policy reloaded = workflow::load_policy(path);
  std::cout << "drop_columns match:    " << std::boolalpha
            << (reloaded.drop_columns == policy.drop_columns) << "\n";
  std::cout << "require_columns match: "
            << (reloaded.require_columns == policy.require_columns) << "\n";
  std::cout << "filter_nulls match:    " << (reloaded.filter_nulls == policy.filter_nulls) << "\n";

  std::filesystem::remove(path + ".policy");

  return 0;
}
```

Output

```text
drop_columns match:    true
require_columns match: true
filter_nulls match:    true
```

## See Also

- [`models::Model::save`](../../models/Model/save.md) — the bundle format
  and `FlowBundleMetadata` fields.
- [`make`](../make.md) — `workflow::load`/`restore`, the reload side of this
  page.
- [`Policy`](../Policy.md) — the object persisted as the sidecar file.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on.html">
    <span class="label">Previous</span>
    <span class="title">on</span>
  </a>
  <a class="next-link" href="with.html">
    <span class="label">Next</span>
    <span class="title">with</span>
  </a>
</div>
<!-- apiref:nav:end -->
