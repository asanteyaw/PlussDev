# `pluss::ai::nn::GELU`

The Gaussian Error Linear Unit, applied elementwise: `0.5 * x * (1 + erf(x /
sqrt(2)))` by default. Like [`SiLU`](SiLU.md), it is smooth and
non-monotonic (it dips slightly below `0` for small negative inputs before
flattening toward `0`), and is the standard activation inside transformer
feed-forward blocks. An optional tanh-based approximation trades a small
amount of accuracy for speed.

## Signature

```cpp
explicit nn::GELU::GELU(nn::GELUOptions options = {});
```

## Construction

Default-construct for the exact `erf`-based curve (`approximate=false`), or
pass a [`GELUOptions`](GELUOptions.md) with `approximate=true` for the
tanh-based approximation.

## State

`options()` (mutable and `const` overloads) returns the
[`GELUOptions`](GELUOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.gelu(options().approximate)`. Never throws.

`options()` -- `GELUOptions&` (or `const GELUOptions&`)
: The options this instance holds.

## Practical Examples

Three examples: the exact curve's shape, comparing exact against the tanh
approximation, and confirming `GELU` runs on the MPS backend.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}, f64);

  nn::GELU gelu;  // approximate=false (default)
  const pluss::Series y = gelu.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "gelu(x) = " << y << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0.5, 2]
gelu(x) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.045500264, -0.15426877, 0, 0.34573123, 1.9544997]
```

### Example 2

The exact and tanh-approximated forms agree to several decimal places but
are not bit-identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}, f64);

  nn::GELU exact;              // approximate=false
  nn::GELUOptions approx_opts{};
  approx_opts.approximate = true;
  nn::GELU approx(approx_opts);

  std::cout << "exact:       " << exact.forward(x) << "\n";
  std::cout << "approximate: " << approx.forward(x) << "\n";

  return 0;
}
```

Output

```text
exact:       pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.045500264, -0.15426877, 0, 0.34573123, 1.9544997]
approximate: pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.045402306, -0.15428599, 0, 0.34571401, 1.9545977]
```

### Example 3

`GELU` runs on the MPS backend the same way it does on CPU -- move the input
to `pMPS` first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}).to(ScalarType::Float32).to(pluss::pMPS);

  nn::GELU gelu;
  try {
    const Series y = gelu.forward(x);
    std::cout << "gelu on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "gelu on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
gelu on MPS: OK, platform=2
```

## Changelog

- **0.1** — the Gaussian Error Linear Unit, `approximate` option.

## See Also

- [`GELUOptions`](GELUOptions.md) -- the options struct.
- [`SiLU`](SiLU.md) -- another smooth, non-monotonic modern activation.
- [`Elu`](Elu.md) -- a saturating (rather than asymptotically linear)
  alternative for the negative side.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

GELUOptions
```
<!-- apiref:toctree:end -->
