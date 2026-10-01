# `pluss::ai::ice::Model::save`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
void pluss::ai::ice::Model::save(std::string_view filepath) const;
```

## What It Does

Persists every registered parameter (the raw `Series`/`nn::Module`
members a subclass registered via `register_parameter`/
`register_module`) via [`ice::io::save_parameters`](../../io/save_parameters.md)
-- the same generic, versioned, checksummed parameter-list artifact
format any `ice` code can use directly. When the model was trained with
[`TrainOptions::standardize = true`](../../options/TrainOptions.md),
the fitted per-column mean/std are also written, to a small sidecar
artifact (`<filepath>.standardize`) alongside the main one.

## Parameters

`std::string_view filepath`
: Where to write the artifact.

## Returns

`void`.

## Errors

- `filepath` cannot be opened for writing -- throws.

## Practical Examples

Three examples: a basic save/load round trip, confirming predictions
match exactly afterward, and the standardize-stats sidecar.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <cstdio>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  model.save("model_save_doc_example.artifact");
  std::cout << "saved\n";
  std::remove("model_save_doc_example.artifact");

  return 0;
}
```

Output

```text
saved
```

### Example 2

A full round trip: save, construct a fresh model with the same
architecture, `load` into it, and confirm predictions match exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <cstdio>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const char* path = "model_save_doc_roundtrip.artifact";
  model.save(path);
  const auto before = model.infer(x);

  ice::Outline loaded(nn::Fusion(2, 1));
  loaded.load(path);
  std::remove(path);

  const auto after = loaded.infer(x);
  const double diff = before.sub(after).abs().amax().item<double>();
  std::cout << "max diff = " << diff << "\n";

  return 0;
}
```

Output

```text
max diff = 0
```

### Example 3

A model trained with `standardize = true` writes a `.standardize`
sidecar alongside the main artifact.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/nn.h>

#include <cstdio>
#include <fstream>
#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const DataFrame df({
      {"a", Thread(pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}))},
      {"b", Thread(pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}))},
      {"label", Thread(pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4}))},
  });

  ice::Outline model(nn::Fusion(2, 1));
  ice::TrainOptions options{};
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.standardize = true;
  model.learn(df, "label", options);

  const char* path = "model_save_doc_standardize.artifact";
  model.save(path);

  std::ifstream sidecar(std::string(path) + ".standardize");
  std::cout << "sidecar exists = " << sidecar.good() << "\n";

  std::remove(path);
  std::remove((std::string(path) + ".standardize").c_str());

  return 0;
}
```

Output

```text
sidecar exists = 1
```

## See Also

- [`load`](load.md) -- the matching load.
- [`save_parameters`](../../io/save_parameters.md) -- the generic
  primitive this calls.
- [`TrainOptions::standardize`](../../options/TrainOptions.md) -- what
  triggers the sidecar artifact.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
