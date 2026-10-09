<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55adfa2d6257 -->
# `pluss::ai::ice::Model::load`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
void pluss::ai::ice::Model::load(std::string_view filepath);
```

## What It Does

Restores trained values into an already-constructed model with an
identical parameter list -- same architecture/options as when
[`save`](save.md)d. Construct-then-load, not a polymorphic factory: the
caller is responsible for constructing a model with the same shape
before calling this. Also restores standardization stats from the
`.standardize` sidecar artifact, if one exists next to `filepath`.
Marks the model [`ready()`](ready.md).

## Parameters

`std::string_view filepath`
: The artifact `save` wrote.

## Returns

`void`.

## Errors

- `filepath` cannot be opened, or the artifact is corrupted/truncated
  (checksum mismatch) -- throws.
- The loaded parameter count/shapes don't match this model's own
  registered parameters -- throws.

## Practical Examples

Three examples: a basic round trip, confirming `ready()` becomes `true`,
and the error from loading into a mismatched architecture.

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

  const char* path = "model_load_doc_example.artifact";
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

### Example 2

`ready()` is `false` before `load`, `true` after.

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const char* path = "model_load_doc_ready.artifact";
  model.save(path);

  ice::Outline loaded(nn::Fusion(2, 1));
  std::cout << "ready() before load = " << loaded.ready() << "\n";
  loaded.load(path);
  std::cout << "ready() after load = " << loaded.ready() << "\n";

  std::remove(path);
  std::remove((std::string(path) + ".weights").c_str());

  return 0;
}
```

Output

```text
ready() before load = 0
ready() after load = 1
```

### Example 3

Loading into a model with a different architecture throws.

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
  options.execution.max_epochs = 50;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const char* path = "model_load_doc_mismatch.artifact";
  model.save(path);

  ice::Outline mismatched(nn::Fusion(2, 5));
  try {
    mismatched.load(path);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "threw: " << message.substr(0, message.find('\n')) << "\n";
  }

  std::remove(path);
  std::remove((std::string(path) + ".weights").c_str());

  return 0;
}
```

Output

```text
threw: pluss::ai::ice::io::load_parameters: parameter 0 size mismatch (does the reconstructed model match the saved architecture?).
```

## See Also

- [`save`](save.md) -- the matching save.
- [`ready`](ready.md) -- becomes `true` after a successful `load`.
- [`load_parameters`](../../io/load_parameters.md) -- the generic
  primitive this calls.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn_step.html">
    <span class="label">Previous</span>
    <span class="title">learn_step</span>
  </a>
  <a class="next-link" href="ready.html">
    <span class="label">Next</span>
    <span class="title">ready</span>
  </a>
</div>
<!-- apiref:nav:end -->
