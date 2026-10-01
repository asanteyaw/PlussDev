# `pluss::ai::nn::optim::AdamWGroup`

One per-parameter-group entry for [`AdamW`](AdamW.md)'s per-group
constructor: a parameter subset paired with its own
[`AdamWOptions`](AdamWOptions.md).

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::AdamWOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::AdamWGroup{{&backbone_weight}, nn::optim::AdamWOptions{.learning_rate = 1e-4, .weight_decay = 0.0}};
nn::optim::AdamWGroup{{&head_weight}, nn::optim::AdamWOptions{.learning_rate = 1e-2, .weight_decay = 0.01}};
```

A `vector<AdamWGroup>` of these is passed to [`AdamW`](AdamW.md)'s
per-group constructor overload -- letting different parts of a model use
different learning rates *and* different decoupled weight decay strengths
in one optimizer instance. Every parameter across every group in the list
must be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`AdamWOptions`](AdamWOptions.md)
: This group's own options, independent of every other group's, including
  its own moment estimates per parameter.

## Methods

None -- `AdamWGroup` is a plain data aggregate, consumed only by
[`AdamW`](AdamW.md)'s constructor.

## Practical Examples

One example: two groups with different learning rates and different
weight decay strengths, trained together in one `AdamW` instance.

### Example 1

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
  const Series target = pluss::make_series(1.0F, f32);

  Series backbone = pluss::make_series(5.0F, f32);
  Series head = pluss::make_series(5.0F, f32);

  std::vector<optim::AdamWGroup> groups{
      optim::AdamWGroup{{&backbone}, optim::AdamWOptions{.learning_rate = 1e-3, .weight_decay = 0.0}},
      optim::AdamWGroup{{&head}, optim::AdamWOptions{.learning_rate = 0.5, .weight_decay = 0.01}},
  };
  optim::AdamW optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    optimizer.step(std::vector<Series>{backbone.sub(target), head.sub(target)});
  }
  std::cout << "backbone (lr=1e-3, wd=0):    w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=0.5, wd=0.01):      w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
backbone (lr=1e-3, wd=0):    w = 4.997
head (lr=0.5, wd=0.01):      w = 3.44479
```

## Changelog

- **0.1** — a parameter list paired with its own `AdamWOptions`.

## See Also

- [`AdamW`](AdamW.md) -- the optimizer this configures per group.
- [`AdamWOptions`](AdamWOptions.md) -- the fields each group's own
  `options` carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
