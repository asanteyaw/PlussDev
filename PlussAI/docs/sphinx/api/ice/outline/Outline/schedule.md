<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6ad0b0aaab13 -->
# `pluss::ai::ice::Outline::schedule`

[Back to `pluss::ai::ice::Outline`](../Outline.md)

## Signature

```cpp
pluss::nn::Schedule& pluss::ai::ice::Outline::schedule() noexcept;
const pluss::nn::Schedule& pluss::ai::ice::Outline::schedule() const noexcept;
```

## What It Does

Exposes the [`Schedule`](../../../nn/module/Schedule.md) `Outline` owns
and runs through on every [`forward`](forward.md) call. The const
overload is for introspection -- stage count, names, declared
input/output types -- without touching the composition. The non-const
overload is a live reference into the same object `Outline`'s
constructor built and already registered as a module: calling
[`append`](../../../nn/module/Schedule/append.md)/
[`insert`](../../../nn/module/Schedule/insert.md)/
[`erase`](../../../nn/module/Schedule/erase.md) on it genuinely extends
or reshapes the composition after construction, and any new layer's
parameters are picked up for training the same way the constructor
arguments were -- `Outline` registers the whole `Schedule` once, not
layer by layer, so there is nothing further to wire up.

## Parameters

None.

## Returns

`Schedule&` / `const Schedule&` -- a reference to the owned object, not
a copy. Mutating through the non-const overload mutates the model
itself.

## Errors

None directly; whatever the called `Schedule` method throws propagates
unchanged (for example appending a stage whose declared input type does
not match the current last stage's output type).

## Practical Examples

Three examples: the const overload for introspection, the non-const
overload to extend a schedule after construction, and an end-to-end run
that trains the extended model.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  const ice::Outline model(nn::Fusion(2, 4), nn::Fusion(4, 1));

  std::cout << "stage count = " << model.schedule()->size() << "\n";
  for (std::size_t i = 0; i < model.schedule()->size(); ++i) {
    const auto info = model.schedule()->stage_info(i);
    std::cout << "stage " << i << ": name = " << info.name
               << ", module_stage = " << info.module_stage << "\n";
  }

  return 0;
}
```

Output

```text
stage count = 2
stage 0: name = 0, module_stage = 1
stage 1: name = 1, module_stage = 1
```

### Example 2

`Outline` is constructed with just one layer here, then
`schedule()`'s non-const overload appends a second one -- the model's
effective output shape changes immediately, before any training has
happened.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 4));
  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});
  std::cout << "before append, forward(x).size(1) = " << model.forward(x).size(1) << "\n";

  model.schedule().append(nn::Fusion(4, 1));
  std::cout << "after append, stage count = " << model.schedule()->size() << "\n";
  std::cout << "after append, forward(x).size(1) = " << model.forward(x).size(1) << "\n";

  return 0;
}
```

Output

```text
before append, forward(x).size(1) = 4
after append, stage count = 2
after append, forward(x).size(1) = 1
```

### Example 3

`learn` on the extended model trains every appended layer's parameters
too -- `Outline` registered the `Schedule` object itself, not a
snapshot of the two constructor-time layers, so the third layer appended
here is already part of what `learn` optimizes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 4), nn::Fusion(4, 1));
  model.schedule().append(nn::Fusion(1, 1));

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  std::cout << "ready() = " << model.ready() << "\n";
  std::cout << "stage count = " << model.schedule()->size() << "\n";

  return 0;
}
```

Output

```text
ready() = 1
stage count = 3
```

## See Also

- [`forward`](forward.md) -- what runs the composed schedule on every
  call.
- [`Schedule::append`](../../../nn/module/Schedule/append.md) /
  [`stage_manifest`](../../../nn/module/Schedule/stage_manifest.md) --
  the mutation and introspection surface this accessor reaches.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward.html">
    <span class="label">Previous</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
