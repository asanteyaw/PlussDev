# `pluss::ai::nn::optim::AdagradGroup`

One per-parameter-group entry for [`Adagrad`](Adagrad.md)'s per-group
constructor: a parameter subset paired with its own
[`AdagradOptions`](AdagradOptions.md).

## Signature

```cpp
std::vector<pluss::Series*> parameters;
nn::optim::AdagradOptions options;
```

A plain aggregate -- construct with `{parameters, options}`, or name each
field.

## Construction

```cpp
nn::optim::AdagradGroup{{&backbone_weight}, nn::optim::AdagradOptions{.learning_rate = 0.1}};
nn::optim::AdagradGroup{{&head_weight}, nn::optim::AdagradOptions{.learning_rate = 1.0}};
```

A `vector<AdagradGroup>` of these is passed to
[`Adagrad`](Adagrad.md)'s per-group constructor overload. Every parameter
across every group in the list must be unique.

## State

`parameters` -- `vector<Series*>`
: This group's own parameter subset.

`options` -- [`AdagradOptions`](AdagradOptions.md)
: This group's own options, independent of every other group's, including
  its own squared-gradient accumulator per parameter.

## Methods

None -- `AdagradGroup` is a plain data aggregate, consumed only by
[`Adagrad`](Adagrad.md)'s constructor.

## Practical Examples

One example: two groups with different learning rates, trained together
in one `Adagrad` instance.

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

  std::vector<optim::AdagradGroup> groups{
      optim::AdagradGroup{{&backbone}, optim::AdagradOptions{.learning_rate = 0.1}},
      optim::AdagradGroup{{&head}, optim::AdagradOptions{.learning_rate = 1.0}},
  };
  optim::Adagrad optimizer(groups);

  for (int step = 0; step < 3; ++step) {
    optimizer.step(std::vector<Series>{backbone.sub(target), head.sub(target)});
  }
  std::cout << "backbone (lr=0.1): w = " << backbone.item<float>() << "\n";
  std::cout << "head (lr=1.0):     w = " << head.item<float>() << "\n";

  return 0;
}
```

Output

```text
backbone (lr=0.1): w = 4.77364
head (lr=1.0):     w = 2.96727
```

## Changelog

- **0.1** — a parameter list paired with its own `AdagradOptions`.

## See Also

- [`Adagrad`](Adagrad.md) -- the optimizer this configures per group.
- [`AdagradOptions`](AdagradOptions.md) -- the fields each group's own
  `options` carries.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
