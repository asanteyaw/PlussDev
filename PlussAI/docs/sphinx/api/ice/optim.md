# `pluss::ai::ice::optim`

`optim` owns [`make_optimizer`](optim/make_optimizer.md), the one free
function that builds an `nn::optim::Optimizer` with `TrainOptions`'
clip/regularization settings already wired in -- the same construction
[`Coach`](coach.md) uses internally behind every
[`Model::learn`](model/Model/learn.md) call, exposed standalone for a
custom training loop that does not fit `Model`'s generic
single-`Series`-in/single-`Series`-out shape.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`make_optimizer`](optim/make_optimizer.md) | one call: pick an `OptimizerKind`, get back a fully configured `Optimizer` -- learning rate, weight decay, gradient clipping, and non-finite diagnostics all read from one `TrainOptions` value |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
Series w = pluss::make_series({0.5F}, {1}, f32);
const Series x = pluss::make_series({2.0F}, {1}, f32);
const Series target = pluss::make_series({5.0F}, {1}, f32);

const ice::TrainOptions options{};  // Adam, learning_rate = 1e-3
const auto optimizer = ice::make_optimizer({&w}, options);

const Series diff = w.mul(x).sub(target);
diff.mul(diff).mean().reverse();
optimizer->step();

std::cout << "w = " << w.item<float>() << "\n";
```

```text
w = 0.501
```

Reach for `make_optimizer` whenever training needs an optimizer but the
data does not fit a fixed `(x, y)` `Batch` -- a per-step loop with
dynamically shaped inputs, negative sampling, or any other loop where
[`Model::learn`](model/Model/learn.md) itself does not apply, but
`ice`'s default safety net (clip, non-finite diagnostics) is still
wanted.

## Related Pages

- [`model`](model.md) -- `Model::learn`, which builds its own optimizer
  through this exact function.
- [`coach`](coach.md) -- where that internal call happens.
- [`options`](options.md) -- `TrainOptions`, every field this function
  reads.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optim/Introduction
optim/make_optimizer
```
<!-- apiref:toctree:end -->
