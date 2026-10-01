# `pluss::ai::nn::SiLU`

The Sigmoid Linear Unit, also known as Swish, applied elementwise: `x *
sigmoid(x)`. Smooth and non-monotonic (it dips slightly below `0` for small
negative inputs before flattening toward `0`), which in practice trains at
least as well as [`ReLU`](ReLU.md) in many modern architectures while
staying differentiable everywhere, unlike `ReLU`'s sharp corner at `0`.

## Signature

```cpp
nn::SiLU::SiLU();
```

## Construction

`SiLU` takes no options -- default-construct it and use it directly, or add
it straight into a [`Schedule`](../module/Schedule.md) with
[`append`](../module/Schedule/append.md).

## State

None. `SiLU` has no parameters and no configuration; every instance behaves
identically.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.silu()`: elementwise `x * sigmoid(x)`. Never throws.

## Practical Examples

Three examples: the elementwise shape of the function, confirming it
matches its own defining formula computed by hand, and running it on the
MPS backend.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}, f64);

  nn::SiLU silu;
  const pluss::Series y = silu.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "silu(x) = " << y << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0.5, 2]
silu(x) = pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.23840584, -0.18877033, 0, 0.31122967, 1.7615942]
```

### Example 2

`silu(x)` is exactly `x * sigmoid(x)` -- confirmed by computing the right
side directly with [`Sigmoid`](Sigmoid.md) and comparing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = pluss::make_series({-1.0, 0.5, 2.0}, {3}, f64);

  nn::SiLU silu;
  nn::Sigmoid sigmoid;

  const Series via_silu = silu.forward(x);
  const Series via_formula = x.mul(sigmoid.forward(x));

  std::cout << "silu(x)          = " << via_silu << "\n";
  std::cout << "x * sigmoid(x)   = " << via_formula << "\n";
  std::cout << "same values: " << std::boolalpha << via_silu.allclose(via_formula) << "\n";

  return 0;
}
```

Output

```text
silu(x)          = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.26894142, 0.31122967, 1.7615942]
x * sigmoid(x)   = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.26894142, 0.31122967, 1.7615942]
same values: true
```

### Example 3

`SiLU` runs on the MPS backend the same way it does on CPU -- move the input
(and, for parameterized layers, the module) to `pMPS` first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series x = pluss::make_series({-2.0, -0.5, 0.0, 0.5, 2.0}, {5}).to(ScalarType::Float32).to(pluss::pMPS);

  nn::SiLU silu;
  try {
    const Series y = silu.forward(x);
    std::cout << "silu on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "silu on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
silu on MPS: OK, platform=2
```

(`platform=2` is `pluss::pMPS`'s raw enum value -- there is no public
`operator<<` for `Platform` in this reference, so examples that need to show
*which* platform a result landed on print the enum value directly.)

## Changelog

- **0.1** — the Sigmoid Linear Unit / Swish activation, no options.

## See Also

- [`Sigmoid`](Sigmoid.md) -- the function `SiLU` is built from.
- [`GELU`](GELU.md) -- another smooth, non-monotonic modern activation.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
