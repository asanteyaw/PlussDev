# `pluss::ai::nn::optim::RMSpropGroup`

One per-parameter-group entry for [`RMSprop`](RMSprop.md)'s per-group
constructor: a parameter subset paired with its own
[`RMSpropOptions`](RMSpropOptions.md).

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::RMSpropOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::RMSpropGroup{{&backbone_weight}, nn::optim::RMSpropOptions{.learning_rate = 0.01}};
nn::optim::RMSpropGroup{{&head_weight}, nn::optim::RMSpropOptions{.learning_rate = 0.5}};
```

A `vector<RMSpropGroup>` of these is passed to [`RMSprop`](RMSprop.md)'s
per-group constructor overload. Every parameter across every group in the
list must be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`RMSpropOptions`](RMSpropOptions.md)
: This group's own options, independent of every other group's, including
  its own running averages per parameter.

## Methods

None -- `RMSpropGroup` is a plain data aggregate, consumed only by
[`RMSprop`](RMSprop.md)'s constructor.

## Practical Examples

One example: two groups with different learning rates, trained together
in one `RMSprop` instance.

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

  std::vector<optim::RMSpropGroup> groups{
      optim::RMSpropGroup{{&backbone}, optim::RMSpropOptions{.learning_rate = 0.01}},
      optim::RMSpropGroup{{&head}, optim::RMSpropOptions{.learning_rate = 0.5}},
  };
  optim::RMSprop optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    const Series backbone_residual = backbone.sub(target);
    const Series head_residual = head.sub(target);
    const float loss = backbone_residual.square().mul(0.5).item<float>() +
                       head_residual.square().mul(0.5).item<float>();
    optimizer.step(std::vector<Series>{backbone_residual, head_residual});
    std::cout << "step " << step << ": loss = " << loss << "\n";
  }
  std::cout << "backbone (lr=0.01): w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=0.5):      w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
step 0: loss = 16
step 1: loss = 8.105
step 2: loss = 7.35835
backbone (lr=0.01): w = 4.77317
head (lr=0.5):      w = 0.951331
```

## Changelog

- **0.1** — a parameter list paired with its own `RMSpropOptions`.

## See Also

- [`RMSprop`](RMSprop.md) -- the optimizer this configures per group.
- [`RMSpropOptions`](RMSpropOptions.md) -- the fields each group's own
  `options` carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
