<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a5fb49e9a683 -->

# `pluss::ai::nn::optim::Optimizer::parameters`

[Back to `pluss::ai::nn::optim::Optimizer`](../Optimizer.md)

## Signature

```cpp
const std::vector<pluss::Series*>& nn::optim::Optimizer::parameters() const noexcept;
```

## What It Does

Returns the flat parameter list this optimizer was constructed with, in
construction order -- the same list [`step`](step.md)'s `gradients`
overload expects one gradient per entry of, in that same order. The
pointers are the exact ones the constructor was given: dereferencing one
reaches the real parameter `Series` the optimizer mutates in place on
every `step()`, not a copy.

## Parameters

None.

## Returns

`const std::vector<Series*>&` -- a reference into the optimizer's own
storage; valid for the optimizer's lifetime, invalidated only by
destroying the optimizer itself.

## Errors

None. `parameters` is `noexcept` and has no preconditions.

## Practical Examples

Three examples: pointer identity against an explicit constructor list,
correspondence with a `Module`'s own named parameters when constructed
from one, and the flattened order per-group construction produces.

### Example 1

Constructing an optimizer from an explicit `vector<Series*>` -- every
pointer `parameters()` returns is the exact same pointer the constructor
was given, in the same order.

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
  Series c = pluss::make_series(3.0F, f32);
  const std::vector<Series*> original{&a, &b, &c};

  nn::optim::SGD optimizer(original);

  std::cout << "size() = " << optimizer.size() << "\n";
  const auto& params = optimizer.parameters();
  for (std::size_t i = 0; i < params.size(); ++i) {
    std::cout << "parameters()[" << i << "] = " << params[i]->item<float>()
               << ", same pointer as constructor arg = " << std::boolalpha << (params[i] == original[i]) << "\n";
  }

  return 0;
}
```

Output

```text
size() = 3
parameters()[0] = 1, same pointer as constructor arg = true
parameters()[1] = 2, same pointer as constructor arg = true
parameters()[2] = 3, same pointer as constructor arg = true
```

### Example 2

Constructing from a `Module&` (via dereferencing a `ModuleHolder`-wrapped
layer): `parameters()` returns the exact same pointers as the layer's own
[`named_parameters()`](../../module/Module/named_parameters.md), one entry
per registered `weight`/`bias`, in registration order.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::ScalarType;

  const pluss::Config f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  nn::Fusion model(3, 2, /*bias=*/true, f32);

  nn::optim::SGD optimizer(*model);

  std::cout << "optimizer.size() = " << optimizer.size() << "\n";
  const auto& named = model->named_parameters();
  for (std::size_t i = 0; i < named.size(); ++i) {
    std::cout << "parameters()[" << i << "] corresponds to '" << named[i].name
               << "', same pointer = " << std::boolalpha << (optimizer.parameters()[i] == named[i].series) << "\n";
  }

  return 0;
}
```

Output

```text
optimizer.size() = 2
parameters()[0] corresponds to 'weight', same pointer = true
parameters()[1] corresponds to 'bias', same pointer = true
```

### Example 3

Constructing from a list of [`SGDGroup`](../SGDGroup.md) values instead of
a flat parameter list: `parameters()` still returns one flat list, in
group order, then in each group's own list order -- the grouping only
affects which `Options` each parameter trains under, never the shape of
`parameters()` itself.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series a = pluss::make_series(1.0F, f32);
  Series b = pluss::make_series(2.0F, f32);
  Series c = pluss::make_series(3.0F, f32);

  std::vector<optim::SGDGroup> groups{
      optim::SGDGroup{{&a, &b}, optim::SGDOptions{.learning_rate = 0.1}},
      optim::SGDGroup{{&c}, optim::SGDOptions{.learning_rate = 0.01}},
  };
  optim::SGD optimizer(groups);

  std::cout << "parameters().size() = " << optimizer.parameters().size() << "\n";
  for (auto* p : optimizer.parameters()) {
    std::cout << "value = " << p->item<float>() << "\n";
  }

  return 0;
}
```

Output

```text
parameters().size() = 3
value = 1
value = 2
value = 3
```

## See Also

- [`size`](size.md) -- `parameters().size()`.
- [`step`](step.md) -- the `gradients` overload expects entries in exactly
  this order.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="last_step_lane_telemetry.html">
    <span class="label">Previous</span>
    <span class="title">last_step_lane_telemetry</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
