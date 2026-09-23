# `pluss::ai::nn::LeakyReLU`

[`ReLU`](ReLU.md) with a small non-zero slope for negative inputs instead of
flattening them to `0`: `x >= 0 ? x : negative_slope * x`. Unlike plain
`ReLU`, a negative input's gradient here is `negative_slope`, not exactly
`0` -- this avoids the "dying ReLU" failure mode where a unit that only ever
sees negative pre-activations stops receiving any gradient at all and can
never recover.

## Signature

```cpp
explicit nn::LeakyReLU::LeakyReLU(nn::LeakyReLUOptions options = {});
```

## Construction

Default-construct for `negative_slope=0.01`, or pass a
[`LeakyReLUOptions`](LeakyReLUOptions.md) to change it.

## State

`options()` (mutable and `const` overloads) returns the
[`LeakyReLUOptions`](LeakyReLUOptions.md) this instance was constructed
with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.leaky_relu(options().negative_slope)`. Never throws.

`options()` -- `LeakyReLUOptions&` (or `const LeakyReLUOptions&`)
: The options this instance holds.

## Practical Examples

Three examples: the default slope, a much steeper custom slope, and a direct
side-by-side with `ReLU` on the same negative input.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::LeakyReLU leaky;  // negative_slope=0.01 (default)
  const pluss::Series y = leaky.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "leaky_relu(x) = " << y << " -- negative entries scaled by 0.01, not zeroed\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, 0, 2]
leaky_relu(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.02, 0, 2] -- negative entries scaled by 0.01, not zeroed
```

### Example 2

A much steeper slope -- half the negative magnitude survives instead of 1%.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::LeakyReLUOptions opts{};
  opts.negative_slope = 0.5;
  nn::LeakyReLU leaky(opts);
  const pluss::Series y = leaky.forward(x);

  std::cout << "negative_slope=0.5: leaky_relu(x) = " << y << "\n";

  return 0;
}
```

Output

```text
negative_slope=0.5: leaky_relu(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 2]
```

### Example 3

The same negative input through `ReLU` (flattened to `0`, zero gradient)
and `LeakyReLU` (a small negative value survives, gradient still flows).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-4.0}, {1}, f64);

  nn::ReLU relu;
  nn::LeakyReLU leaky;  // negative_slope=0.01

  std::cout << "relu(-4)       = " << relu.forward(x) << " -- the gradient here is exactly 0\n";
  std::cout << "leaky_relu(-4) = " << leaky.forward(x) << " -- small but nonzero, so gradient still flows\n";

  return 0;
}
```

Output

```text
relu(-4)       = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0] -- the gradient here is exactly 0
leaky_relu(-4) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.04] -- small but nonzero, so gradient still flows
```

## Changelog

- **0.1** — the leaky variant of `ReLU`, `negative_slope` option.

## See Also

- [`LeakyReLUOptions`](LeakyReLUOptions.md) -- the options struct.
- [`ReLU`](ReLU.md) -- the `negative_slope = 0` limiting case.
- [`Elu`](Elu.md) -- a smooth (not piecewise-linear) alternative with the
  same "keep negative inputs alive" motivation.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LeakyReLUOptions
```
<!-- apiref:toctree:end -->
