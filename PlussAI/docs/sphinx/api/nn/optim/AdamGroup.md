# `pluss::ai::nn::optim::AdamGroup`

One per-parameter-group entry for [`Adam`](Adam.md)'s per-group
constructor: a parameter subset paired with its own
[`AdamOptions`](AdamOptions.md).

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::AdamOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::AdamGroup{{&backbone_weight}, nn::optim::AdamOptions{.learning_rate = 1e-4}};
nn::optim::AdamGroup{{&head_weight}, nn::optim::AdamOptions{.learning_rate = 1e-2}};
```

A `vector<AdamGroup>` of these is passed to [`Adam`](Adam.md)'s per-group
constructor overload -- the common way to give a pretrained backbone a
smaller learning rate than a freshly initialized head, in one optimizer
instance. Every parameter across every group in the list must be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`AdamOptions`](AdamOptions.md)
: This group's own options, independent of every other group's, including
  its own first/second moment estimates per parameter.

## Methods

None -- `AdamGroup` is a plain data aggregate, consumed only by
[`Adam`](Adam.md)'s constructor.

## Practical Examples

One example: two groups with very different learning rates, trained
together in one `Adam` instance.

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

  std::vector<optim::AdamGroup> groups{
      optim::AdamGroup{{&backbone}, optim::AdamOptions{.learning_rate = 1e-3}},
      optim::AdamGroup{{&head}, optim::AdamOptions{.learning_rate = 0.5}},
  };
  optim::Adam optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    optimizer.step(std::vector<Series>{backbone.sub(target), head.sub(target)});
  }
  std::cout << "backbone (lr=1e-3): w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=0.5):      w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
backbone (lr=1e-3): w = 4.997
head (lr=0.5):      w = 3.5111
```

## Changelog

- **0.1** — a parameter list paired with its own `AdamOptions`.

## See Also

- [`Adam`](Adam.md) -- the optimizer this configures per group.
- [`AdamOptions`](AdamOptions.md) -- the fields each group's own `options`
  carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
