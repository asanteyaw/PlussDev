# `pluss::ai::nn::optim::SGDGroup`

One per-parameter-group entry for [`SGD`](SGD.md)'s per-group constructor:
a parameter subset paired with its own [`SGDOptions`](SGDOptions.md), so
different parts of a model can train under different learning rates,
momentum, decay, or clipping within a single `SGD` instance.

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::SGDOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::SGDGroup{{&backbone_weight, &backbone_bias}, nn::optim::SGDOptions{.learning_rate = 0.01}};
nn::optim::SGDGroup{{&head_weight}, nn::optim::SGDOptions{.learning_rate = 0.5}};
```

A `vector<SGDGroup>` of these is passed to
[`SGD`](SGD.md)'s per-group constructor overload. Every parameter across
every group in the list must be unique -- `SGD` throws on a parameter
pointer that appears in more than one group.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`SGDOptions`](SGDOptions.md)
: This group's own options, independent of every other group's.

## Methods

None -- `SGDGroup` is a plain data aggregate, consumed only by
[`SGD`](SGD.md)'s constructor.

## Practical Examples

One example: two groups with different learning rates, trained together
in one `SGD` instance, converging at visibly different rates.

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

  std::vector<optim::SGDGroup> groups{
      optim::SGDGroup{{&backbone}, optim::SGDOptions{.learning_rate = 0.01}},
      optim::SGDGroup{{&head}, optim::SGDOptions{.learning_rate = 0.5}},
  };
  optim::SGD optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    optimizer.step(std::vector<Series>{backbone.sub(target), head.sub(target)});
  }
  std::cout << "backbone (lr=0.01): w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=0.5):      w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
backbone (lr=0.01): w = 4.8812
head (lr=0.5):      w = 1.5
```

The `gradients` overload's own single flat list still applies across both
groups in group order (`backbone`'s gradient first, then `head`'s) --
per-group construction changes which options apply to each parameter,
never the shape of `step`'s own arguments.

## Changelog

- **0.1** — a parameter list paired with its own `SGDOptions`.

## See Also

- [`SGD`](SGD.md) -- the optimizer this configures per group.
- [`SGDOptions`](SGDOptions.md) -- the fields each group's own `options`
  carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
