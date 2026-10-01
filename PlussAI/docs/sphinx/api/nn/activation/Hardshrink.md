# `pluss::ai::nn::Hardshrink`

The hard shrinkage function, applied elementwise: `|x| > lambd ? x : 0`.
Unlike every other activation in this family, `Hardshrink` does not squash
or reshape its input at all outside the dead zone -- values that survive
pass through completely unchanged, and everything within `[-lambd, lambd]`
of `0` is zeroed exactly. Built on `where`, so it is differentiable
everywhere except at the two threshold points.

## Signature

```cpp
explicit nn::Hardshrink::Hardshrink(nn::HardshrinkOptions options);
explicit nn::Hardshrink::Hardshrink(double lambd = 0.5);
```

## Construction

Two equivalent forms: pass a
[`HardshrinkOptions`](HardshrinkOptions.md) directly, or spell out
`lambd` as a positional argument (which defaults to `0.5`, so
`nn::Hardshrink hs;` alone works):

```cpp
nn::Hardshrink a(nn::HardshrinkOptions{1.0});  // lambd=1.0
nn::Hardshrink b(1.0);                          // equivalent positional form
nn::Hardshrink c;                               // lambd=0.5 (default)
```

## State

`options()` (mutable and `const` overloads) returns the
[`HardshrinkOptions`](HardshrinkOptions.md) this instance was constructed
with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.hardshrink(options().lambd)`. Never throws.

`options()` -- `HardshrinkOptions&` (or `const HardshrinkOptions&`)
: The options this instance holds.

## Practical Examples

Three examples: the default zeroing band, a wider band from a custom
`lambd`, and confirming `Hardshrink` runs on the MPS backend.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}, f64);

  nn::Hardshrink hardshrink;  // lambd=0.5 (default)
  const pluss::Series y = hardshrink.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "hardshrink(x) = " << y << " -- everything in [-0.5, 0.5] is zeroed\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0.5, 2]
hardshrink(x) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, 0, 0, 0, 2] -- everything in [-0.5, 0.5] is zeroed
```

Note that `-0.5` and `0.5` themselves are zeroed too: the surviving
condition is a strict `|x| > lambd`, not `>=`.

### Example 2

A larger `lambd` widens the zeroed band.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -1.0, 0.0, 1.0, 2.0}, {5}, f64);

  nn::HardshrinkOptions opts{};
  opts.lambd = 1.5;
  nn::Hardshrink hardshrink(opts);
  const pluss::Series y = hardshrink.forward(x);

  std::cout << "lambd=1.5: hardshrink(x) = " << y << "\n";

  return 0;
}
```

Output

```text
lambd=1.5: hardshrink(x) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, 0, 0, 0, 2]
```

### Example 3

`Hardshrink` runs on the MPS backend the same way it does on CPU -- move the
input to `pMPS` first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}).to(ScalarType::Float32).to(pluss::pMPS);

  nn::Hardshrink hardshrink;
  try {
    const Series y = hardshrink.forward(x);
    std::cout << "hardshrink on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "hardshrink on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
hardshrink on MPS: OK, platform=2
```

## Changelog

- **0.1** — the hard shrinkage function, `lambd` option.
- **0.2** — added the positional `Hardshrink(lambd)` constructor
  alongside the `HardshrinkOptions` form (previously, a bare numeric
  argument silently bound to the inherited `Module(bool training)`
  constructor instead of `lambd`, since no positional form existed to
  compete with it).

## See Also

- [`HardshrinkOptions`](HardshrinkOptions.md) -- the options struct.
- [`LeakyReLU`](LeakyReLU.md) -- another piecewise-linear activation, but
  one that never zeroes its input.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

HardshrinkOptions
```
<!-- apiref:toctree:end -->
