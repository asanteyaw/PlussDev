# `pluss::ai::nn::ReLU`

The rectified linear unit, applied elementwise: `max(0, x)`. The default
activation to reach for between fully connected or convolutional layers --
cheap to compute, and its gradient is either exactly `0` or exactly `1`
(never vanishingly small the way `Sigmoid`/`Tanh`'s can be for large inputs),
which is why deep networks train more reliably with it than with the older
saturating activations. The one cost: any input that lands at or below `0`
produces a gradient of exactly `0` too -- see [`LeakyReLU`](LeakyReLU.md) for
a variant that keeps a small gradient flowing there.

## Signature

```cpp
nn::ReLU::ReLU();
```

## Construction

`ReLU` takes no options -- default-construct it and use it directly, or add
it straight into a [`Schedule`](../module/Schedule.md) with
[`then`](../module/Schedule/then.md).

## State

None. `ReLU` has no parameters and no configuration; every instance behaves
identically.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.relu()`: elementwise `max(0, x)`. Never throws.

## Practical Examples

Two examples: the elementwise shape of the function on a mixed-sign vector,
and `ReLU` used as the activation stage in a small `Schedule`, contrasting
the fully connected layer's raw output against the activated one.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}, f64);

  nn::ReLU relu;
  const pluss::Series y = relu.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "relu(x) = " << y << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0.5, 2]
relu(x) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 0, 0.5, 2]
```

### Example 2

A `Fusion` layer's pre-activation output has one negative entry; adding
`ReLU` as the next `Schedule` stage clips exactly that entry to `0` and
leaves the rest unchanged.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  nn::Fusion fc(2, 3, /*bias=*/false, f64);
  const pluss::Series w = pluss::make_series({1.0, 0.0, -1.0, 0.0, 0.5, 0.5}, {3, 2}, f64);
  fc->weight().copy_(w);

  nn::Schedule net;
  net.then("fc", fc);
  net.then("act", nn::ReLU());

  const pluss::Series x = pluss::make_series({2.0, 1.0}, {1, 2}, f64);
  std::cout << "fc(x) (pre-activation) = " << fc.forward(x) << "\n";
  std::cout << "net(x) (after ReLU)    = " << net.forward(x) << "\n";

  return 0;
}
```

Output

```text
fc(x) (pre-activation) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[2, -2, 1.5]]
net(x) (after ReLU)    = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[2, 0, 1.5]]
```

## Changelog

- **0.1** — the base elementwise `max(0, x)` activation, no options.

## See Also

- [`LeakyReLU`](LeakyReLU.md) -- keeps a small gradient for negative inputs
  instead of zeroing it.
- [`Elu`](Elu.md) -- a smooth alternative that saturates rather than
  clipping negative inputs.
- [`Softplus`](Softplus.md) -- a fully smooth (differentiable everywhere)
  approximation of `ReLU`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
