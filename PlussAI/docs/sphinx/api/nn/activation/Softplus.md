# `pluss::ai::nn::Softplus`

A smooth approximation of [`ReLU`](ReLU.md): `(1/beta) * log(1 + exp(beta *
x))`, differentiable everywhere (unlike `ReLU`'s sharp corner at `0`) and
never exactly `0` for any finite input. Falls back to the plain linear
identity `x` once `beta * x` exceeds `threshold`, both to stay numerically
stable (the un-approximated formula would risk overflow computing
`exp(beta * x)` for large `x`) and because the approximation is
indistinguishable from `x` itself out there anyway.

## Signature

```cpp
explicit nn::Softplus::Softplus(nn::SoftplusOptions options);
explicit nn::Softplus::Softplus(double beta = 1.0, double threshold = 20.0);
```

## Construction

Two equivalent forms: pass a [`SoftplusOptions`](SoftplusOptions.md)
directly, or spell out `beta`/`threshold` as positional arguments (both
default, so `nn::Softplus sp;` alone works):

```cpp
nn::Softplus a(nn::SoftplusOptions{2.0, 15.0});  // beta=2.0, threshold=15.0
nn::Softplus b(2.0, 15.0);                        // equivalent positional form
nn::Softplus c;                                   // beta=1.0, threshold=20.0 (defaults)
```

## State

`options()` (mutable and `const` overloads) returns the
[`SoftplusOptions`](SoftplusOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.softplus(options().beta, options().threshold)`. Never
  throws.

`options()` -- `SoftplusOptions&` (or `const SoftplusOptions&`)
: The options this instance holds.

## Practical Examples

Three examples: the default curve, a larger `beta` sharpening it toward
`ReLU`, and an input large enough to trigger the linear fallback.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::Softplus softplus;  // beta=1.0, threshold=20.0 (defaults)
  const pluss::Series y = softplus.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "softplus(x) = " << y << " -- a smooth approximation of ReLU, never exactly 0\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, 0, 2]
softplus(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.12692801, 0.69314718, 2.126928] -- a smooth approximation of ReLU, never exactly 0
```

### Example 2

Raising `beta` sharpens the transition -- as `beta` grows, `softplus`
converges toward `ReLU`'s sharp corner at `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::SoftplusOptions sharp{};
  sharp.beta = 10.0;
  nn::Softplus softplus(sharp);
  const pluss::Series y = softplus.forward(x);

  std::cout << "beta=10: softplus(x) = " << y << " -- a larger beta sharpens the curve toward ReLU\n";

  return 0;
}
```

Output

```text
beta=10: softplus(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0611536e-10, 0.069314718, 2] -- a larger beta sharpens the curve toward ReLU
```

### Example 3

`beta * x = 1.0 * 30.0 = 30.0`, above the default `threshold` of `20.0` --
`softplus` falls back to the linear identity instead of computing
`log(1 + exp(30))`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({30.0}, {1}, f64);

  nn::Softplus softplus;  // threshold=20.0 (default)
  const pluss::Series y = softplus.forward(x);

  std::cout << "softplus(30) = " << y << " -- above threshold, falls back to x itself\n";

  return 0;
}
```

Output

```text
softplus(30) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[30] -- above threshold, falls back to x itself
```

## Changelog

- **0.1** — the smooth `ReLU` approximation, `beta`/`threshold` options.
- **0.2** — added the positional `Softplus(beta, threshold)` constructor
  alongside the `SoftplusOptions` form (previously, a bare numeric
  argument silently bound to the inherited `Module(bool training)`
  constructor instead of `beta`, since no positional form existed to
  compete with it).

## See Also

- [`SoftplusOptions`](SoftplusOptions.md) -- the options struct.
- [`ReLU`](ReLU.md) -- the sharp-cornered function `Softplus` approximates.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SoftplusOptions
```
<!-- apiref:toctree:end -->
