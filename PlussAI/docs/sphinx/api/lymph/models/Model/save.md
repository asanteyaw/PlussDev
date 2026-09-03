<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7780be293742 -->

# `lymph::models::Model::save`

[Back to `lymph::models::Model`](../Model.md)

`save` writes a trained model to a single bundle file: its fitted preparation, its
estimator parameters, and — optionally — the training batch and a metadata stamp.
[`load`](load.md) reads it back.

## Signature

```cpp
void lymph::models::Model::save(const std::string& path) const;

void lymph::models::Model::save(
    const std::string& path,
    lymph::io::FlowBundleMetadata metadata) const;

void lymph::models::Model::save(
    const std::string& path,
    const pluss::table::DataFrame& x_train,
    const pluss::Series& y_train,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;

void lymph::models::Model::save(
    const std::string& path,
    const pluss::table::DataFrame& x_train,
    const pluss::table::Thread& y_train,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;

void lymph::models::Model::save(
    const std::string& path,
    const pluss::table::DataFrame& frame,
    const lymph::options::DFBatch& data_options,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;

void lymph::models::Model::save(
    const std::string& path,
    const lymph::LabeledBatch& train_batch,
    const lymph::options::Train& train_options = {},
    lymph::io::FlowBundleMetadata metadata = {}) const;
```

## What It Does

`save` serialises the fitted flow. The minimum — the one-argument form — is
everything needed to reproduce the model's predictions: every fitted preparation
stage (a scaler's centre and spread, an encoder's vocabulary) and the estimator's
learned parameters. That bundle reloads into a `Model` that predicts identically
(Example 1).

The larger overloads additionally embed the **training data** and the
`TrainOptions` it was fitted with, so the artifact is self-describing: it records
not only *what* the model learned but *what from*. This makes the bundle bigger
(Example 2) and is worth it when you need to audit or re-fit the model later
without hunting for the original inputs.

Every overload also accepts a `FlowBundleMetadata` stamp identifying the
component and version that produced the bundle (Example 3) — set these for your
own service so a stored artifact traces back to the run that made it.

`save` operates on a *trained* model. Saving a template is an error; there is
nothing fitted to write.

## Parameters

`const std::string& path`
: Where to write the bundle. A single file; the containing directory must exist.
  A common convention is a `.flow` extension.

`const pluss::table::DataFrame& x_train, const pluss::Series& y_train`
: The training inputs to embed. Supplying them switches on the self-describing
  form. The label may also be a `pluss::table::Thread` (`y_train`), or the
  features and label may arrive as one `frame` plus a
  `data_options` (`lymph::options::DFBatch`) naming the label column, or as a
  single `train_batch` (`lymph::LabeledBatch`). Use the shape you trained with.

`const lymph::options::Train& train_options` — default `{}`
: The training options to record alongside the embedded data, so a later re-fit
  can reproduce the run.

`lymph::io::FlowBundleMetadata metadata` — default `{}`
: A provenance stamp. Its fields:

  `std::string producer`
  : The component that wrote the bundle. Defaults to a generic internal
    identifier; set it to your service name.

  `std::string producer_version` — default `"0.1.0"`
  : That component's version.

  `std::uint32_t schema_version`
  : The bundle-format version, set for you; `load` checks it.

  `std::uint64_t created_unix_nanos`
  : A creation timestamp you may fill.

## Returns

Nothing. On success the bundle file exists at `path`.

## Errors

- Saving an untrained model, or a `learn` template, raises.
- An unwritable `path` (missing directory, no permission) raises an I/O error.
- Passing embedded training data of a different shape than the model was fitted
  with is rejected.

## Practical Examples

### Example 1

The minimal save. Train, write the one-argument bundle, reload it, and confirm
predictions are byte-for-byte the same.

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

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // The one-argument form writes the fitted flow -- preparation state and
  // estimator parameters -- to a single bundle file.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model.flow";
  model.save(path.string());
  std::cout << "wrote " << std::filesystem::file_size(path) << " bytes\n";

  // Reload it into a brand new Model and confirm predictions are unchanged.
  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  const DataFrame check({
      {"income", Thread(income)},
      {"original", Thread(model.infer(x))},
      {"reloaded", Thread(reloaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
wrote 490 bytes
=================================
     | income  original  reloaded
---------------------------------
0    | 1       0         0       
1    | 1.4     0         0       
2    | 1.8     0         0       
3    | 3.4     1         1       
4    | 3.8     1         1       
5    | 4.2     1         1       
---------------------------------
[6 cs x 3 feat]
```

### Example 2

Embedding the training data. The bundle that carries the data is larger than the
plain one; the reload keeps the two preparation stages.

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

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.prepare().scale({"income", "balance"});
  model.learn_(x, y);

  // The overloads that also take the training data embed that batch in the
  // bundle, so the saved artifact is self-describing: it records what it was
  // fitted on, not only the fitted parameters.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model_with_data.flow";
  model.save(path.string(), x, y);

  const std::uintmax_t plain_size = [&] {
    const std::filesystem::path p2 = std::filesystem::temp_directory_path() / "lymph_model_plain.flow";
    model.save(p2.string());
    const std::uintmax_t s = std::filesystem::file_size(p2);
    std::filesystem::remove(p2);
    return s;
  }();

  std::cout << "bundle without data = " << plain_size << " bytes\n";
  std::cout << "bundle with data    = " << std::filesystem::file_size(path) << " bytes\n";

  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  std::cout << "reloaded trained = " << std::boolalpha << reloaded.trained()
            << ", stage_count = " << reloaded.stage_count() << "\n";

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
bundle without data = 673 bytes
bundle with data    = 873 bytes
reloaded trained = true, stage_count = 1
```

### Example 3

A provenance stamp. Set `producer` and `producer_version` on a
`FlowBundleMetadata`; the reloaded model still predicts identically.

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

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // A FlowBundleMetadata stamp travels with the bundle: which component wrote
  // it and at what version. Set the producer fields for your own service so a
  // reloaded artifact can be traced back to the run that made it.
  lymph::io::FlowBundleMetadata meta{};
  meta.producer = "credit-scoring-service";
  meta.producer_version = "2.4.1";

  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model_stamped.flow";
  model.save(path.string(), meta);

  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  std::cout << "reloaded trained = " << std::boolalpha << reloaded.trained() << "\n\n";

  const DataFrame check({
      {"income", Thread(income)},
      {"original", Thread(model.infer(x))},
      {"reloaded", Thread(reloaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
```

Output

```text
reloaded trained = true

=================================
     | income  original  reloaded
---------------------------------
0    | 1       0         0       
1    | 1.4     0         0       
2    | 1.8     0         0       
3    | 3.4     1         1       
4    | 3.8     1         1       
5    | 4.2     1         1       
---------------------------------
[6 cs x 3 feat]
```

## See Also

- [load](load.md) — reads a bundle back into a `Model`.
- [learn](learn.md) — must have run before `save`.
- [Introduction](Introduction.md) — persistence notes and lifecycle.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prepare.html">
    <span class="label">Previous</span>
    <span class="title">prepare</span>
  </a>
</div>
<!-- apiref:nav:end -->
