<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ce188d76dbe -->
# `pluss::ai::ice::Model::ready`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
bool pluss::ai::ice::Model::ready() const noexcept;
```

## What It Does

Reports whether [`learn`](learn.md) or [`load`](load.md) has been
called at least once. `false` on a freshly constructed model.

## Parameters

None.

## Returns

`bool`.

## Errors

None. `ready` is `noexcept` and never throws.

## Practical Examples

Three examples: before `learn`, after `learn`, and after `load` on a
fresh model.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  std::cout << "ready() before learn = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() before learn = 0
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 10;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  std::cout << "ready() after learn = " << model.ready() << "\n";

  return 0;
}
```

Output

```text
ready() after learn = 1
```

### Example 3

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
  options.execution.max_epochs = 10;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const char* path = "model_ready_doc_example.artifact";
  model.save(path);

  ice::Outline fresh(nn::Fusion(2, 1));
  fresh.load(path);
  std::cout << "ready() after load = " << fresh.ready() << "\n";

  std::remove(path);
  std::remove((std::string(path) + ".weights").c_str());

  return 0;
}
```

Output

```text
ready() after load = 1
```

## See Also

- [`learn`](learn.md) / [`load`](load.md) -- the two ways `ready()`
  becomes `true`.
- [`is_training`](is_training.md) -- a different, independent flag
  (train vs. eval mode).

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
