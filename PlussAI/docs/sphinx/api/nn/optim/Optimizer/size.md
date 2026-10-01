<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 97501351b6eb -->

# `pluss::ai::nn::optim::Optimizer::size`

[Back to `pluss::ai::nn::optim::Optimizer`](../Optimizer.md)

## Signature

```cpp
std::size_t nn::optim::Optimizer::size() const noexcept;
```

## What It Does

Returns the number of parameters this optimizer was constructed with --
`parameters().size()`, provided directly so a caller does not need to
reach through `parameters()` just to ask "how many". This is the count
[`step`](step.md)'s `gradients` overload validates its argument against:
passing a gradient list of any other length throws.

## Parameters

None.

## Returns

`std::size_t`. Always `> 0` for a successfully constructed optimizer --
every constructor throws on an empty parameter list, so there is no
"empty optimizer" state to observe.

## Errors

None. `size` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: `size()` alongside `parameters().size()` to confirm they
agree, `size()` reflecting a `Module`'s full registered parameter count
rather than a single layer's own, and the edge case every constructor
rejects -- an empty parameter list.

### Example 1

`size()` and `parameters().size()` always agree -- `size()` exists purely
for convenience.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series a = pluss::make_series(1.0F, f32);
  Series b = pluss::make_series(2.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&a, &b});

  std::cout << "size() = " << optimizer.size() << "\n";
  std::cout << "parameters().size() = " << optimizer.parameters().size() << "\n";

  return 0;
}
```

Output

```text
size() = 2
parameters().size() = 2
```

### Example 2

A two-layer `Schedule`'s combined parameter count -- `size()` reflects
every registered parameter across every child `Fusion`, not just the one
the optimizer might have been pointed at.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  const pluss::Config f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  nn::Schedule net;
  net.append("a", nn::Fusion(3, 4, true, f32));
  net.append("b", nn::Fusion(4, 2, true, f32));

  nn::optim::SGD optimizer(*net);
  std::cout << "size() = " << optimizer.size() << " (2 layers x (weight + bias) each)\n";

  return 0;
}
```

Output

```text
size() = 4 (2 layers x (weight + bias) each)
```

### Example 3

Every optimizer constructor throws on an empty parameter list -- `size()`
is never observably `0` on a successfully constructed optimizer.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  try {
    const std::vector<Series*> empty{};
    nn::optim::SGD optimizer(empty);
  } catch (const std::exception& e) {
    std::cout << "caught: " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::ai::nn::optim::Optimizer: expected non-empty parameter list
```

## See Also

- [`parameters`](parameters.md) -- the list `size()` counts.
- [`step`](step.md) -- the `gradients` overload requires exactly this many
  entries.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="parameters.html">
    <span class="label">Previous</span>
    <span class="title">parameters</span>
  </a>
  <a class="next-link" href="step.html">
    <span class="label">Next</span>
    <span class="title">step</span>
  </a>
</div>
<!-- apiref:nav:end -->
