# `pluss::ai::nn::optim::NAdamGroup`

One per-parameter-group entry for [`NAdam`](NAdam.md)'s per-group
constructor: a parameter subset paired with its own
[`NAdamOptions`](NAdamOptions.md).

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::NAdamOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::NAdamGroup{{&backbone_weight}, nn::optim::NAdamOptions{.learning_rate = 2e-4}};
nn::optim::NAdamGroup{{&head_weight}, nn::optim::NAdamOptions{.learning_rate = 2e-2}};
```

A `vector<NAdamGroup>` of these is passed to [`NAdam`](NAdam.md)'s
per-group constructor overload. Every parameter across every group in the
list must be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`NAdamOptions`](NAdamOptions.md)
: This group's own options, independent of every other group's, including
  its own moment estimates and momentum-schedule accumulator per
  parameter.

## Methods

None -- `NAdamGroup` is a plain data aggregate, consumed only by
[`NAdam`](NAdam.md)'s constructor.

## Practical Examples

One example: two groups with very different learning rates, trained
together in one `NAdam` instance.

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

  std::vector<optim::NAdamGroup> groups{
      optim::NAdamGroup{{&backbone}, optim::NAdamOptions{.learning_rate = 2e-4}},
      optim::NAdamGroup{{&head}, optim::NAdamOptions{.learning_rate = 0.5}},
  };
  optim::NAdam optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    const Series backbone_residual = backbone.sub(target);
    const Series head_residual = head.sub(target);
    const float loss = backbone_residual.square().mul(0.5).item<float>() +
                       head_residual.square().mul(0.5).item<float>();
    optimizer.step(std::vector<Series>{backbone_residual, head_residual});
    std::cout << "step " << step << ": loss = " << loss << "\n";
  }
  std::cout << "backbone (lr=2e-4): w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=0.5):      w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
step 0: loss = 16
step 1: loss = 14.0258
step 2: loss = 12.8202
backbone (lr=2e-4): w = 4.99949
head (lr=0.5):      w = 3.77764
```

## Changelog

- **0.1** — a parameter list paired with its own `NAdamOptions`.

## See Also

- [`NAdam`](NAdam.md) -- the optimizer this configures per group.
- [`NAdamOptions`](NAdamOptions.md) -- the fields each group's own
  `options` carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
