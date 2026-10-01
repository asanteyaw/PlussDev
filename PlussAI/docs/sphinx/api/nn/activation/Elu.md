# `pluss::ai::nn::Elu`

The exponential linear unit, applied elementwise: `x > 0 ? x : alpha *
(exp(x) - 1)`. Like [`LeakyReLU`](LeakyReLU.md), negative inputs keep a
nonzero gradient instead of dying at `0`; unlike `LeakyReLU`, the negative
side is a smooth, saturating curve (bounded below by `-alpha`) rather than a
straight line that grows without limit. A stateless functional form,
[`elu`](../functional/elu.md), is also available for callers that want the
computation without constructing a `Module`.

## Signature

```cpp
explicit nn::Elu::Elu(nn::EluOptions options);
explicit nn::Elu::Elu(double alpha = 1.0);
```

## Construction

Two equivalent forms: pass an [`EluOptions`](EluOptions.md) directly, or
spell out `alpha` as a positional argument (which defaults to `1.0`, so
`nn::Elu elu;` alone works):

```cpp
nn::Elu a(nn::EluOptions{0.5});  // alpha=0.5
nn::Elu b(0.5);                  // equivalent positional form
nn::Elu c;                       // alpha=1.0 (default)
```

## State

`options()` (mutable and `const` overloads) returns the
[`EluOptions`](EluOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.elu(options().alpha)`. Never throws.

`options()` -- `EluOptions&` (or `const EluOptions&`)
: The options this instance holds.

## Practical Examples

Three examples: the default saturation toward `-alpha`, a smaller `alpha`
saturating closer to `0`, and confirming the `Module` form and the
[functional form](../functional/elu.md) agree.

### Example 1

Negative inputs saturate toward `-alpha` as they grow more negative, never
reaching it exactly and never crossing `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::Elu elu;  // alpha=1.0 (default)
  const pluss::Series y = elu.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "elu(x) = " << y << " -- negative entries saturate toward -alpha, never zero\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, 0, 2]
elu(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.86466472, 0, 2] -- negative entries saturate toward -alpha, never zero
```

### Example 2

A smaller `alpha` pulls the saturation floor closer to `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0}, {1}, f64);

  nn::EluOptions opts{};
  opts.alpha = 0.5;
  nn::Elu elu(opts);
  const pluss::Series y = elu.forward(x);

  std::cout << "alpha=0.5: elu(-2) = " << y << " -- saturates toward -0.5 instead of -1.0\n";

  return 0;
}
```

Output

```text
alpha=0.5: elu(-2) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.43233236] -- saturates toward -0.5 instead of -1.0
```

### Example 3

The `Module` form and the free-function form compute exactly the same
values -- `Elu::forward` calls `input.elu(alpha)` directly, the same
operation [`elu`](../functional/elu.md) wraps.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::Elu elu_module;
  const pluss::Series via_module = elu_module.forward(x);
  const pluss::Series via_function = nn::elu(x);  // the free-function form, no Module to construct

  std::cout << "same values: " << std::boolalpha << via_module.allclose(via_function) << "\n";

  return 0;
}
```

Output

```text
same values: true
```

## Changelog

- **0.1** — the exponential linear unit, `alpha` option, plus the
  stateless [`elu`](../functional/elu.md) function form.
- **0.2** — added the positional `Elu(alpha)` constructor alongside the
  `EluOptions` form (previously, a bare numeric argument silently bound
  to the inherited `Module(bool training)` constructor instead of
  `alpha`, since no positional form existed to compete with it).

## See Also

- [`EluOptions`](EluOptions.md) -- the options struct.
- [`elu`](../functional/elu.md) -- the stateless functional form.
- [`LeakyReLU`](LeakyReLU.md) -- the piecewise-linear alternative with a
  similar motivation.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EluOptions
```
<!-- apiref:toctree:end -->
