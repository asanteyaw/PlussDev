# `pluss::ai::nn::Tanh`

Hyperbolic tangent, applied elementwise: `tanh(x)`, squashing any real input
into the open interval `(-1, 1)`. An odd function (`tanh(-x) == -tanh(x)`)
centered at `0`, unlike [`Sigmoid`](Sigmoid.md)'s `(0, 1)` range centered at
`0.5` -- prefer `Tanh` over `Sigmoid` as a hidden-layer activation when a
zero-centered output matters for what follows (many classical RNN/GRU gate
formulations, for instance, use `tanh` specifically for this reason).

## Signature

```cpp
nn::Tanh::Tanh();
```

## Construction

`Tanh` takes no options -- default-construct it and use it directly, or add
it straight into a [`Schedule`](../module/Schedule.md) with
[`append`](../module/Schedule/append.md).

## State

None. `Tanh` has no parameters and no configuration; every instance behaves
identically.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.tanh()`: elementwise hyperbolic tangent. Never throws.

## Practical Examples

Two examples: the squashing behavior at `0` and at a moderate magnitude, and
the odd-function symmetry `tanh(-x) == -tanh(x)`.

### Example 1

`0` maps to exactly `0`; positive and negative inputs of the same
magnitude produce values of the same magnitude with opposite sign.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 2.0}, {3}, f64);

  nn::Tanh tanh_layer;
  const pluss::Series y = tanh_layer.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "tanh(x) = " << y << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, 0, 2]
tanh(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.96402758, 0, 0.96402758]
```

### Example 2

`tanh(-x) == -tanh(x)` for any `x` -- confirmed directly rather than assumed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  nn::Tanh tanh_layer;

  const pluss::Series x = pluss::make_series({1.5}, {1}, f64);
  const pluss::Series neg_x = pluss::make_series({-1.5}, {1}, f64);

  const pluss::Series t_x = tanh_layer.forward(x);
  const pluss::Series t_neg_x = tanh_layer.forward(neg_x);

  std::cout << "tanh(1.5)  = " << t_x << "\n";
  std::cout << "tanh(-1.5) = " << t_neg_x << "\n";
  std::cout << "tanh(-x) == -tanh(x): " << std::boolalpha << t_neg_x.allclose(t_x.neg()) << "\n";

  return 0;
}
```

Output

```text
tanh(1.5)  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.90514825]
tanh(-1.5) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-0.90514825]
tanh(-x) == -tanh(x): true
```

## Changelog

- **0.1** — the base elementwise hyperbolic tangent, no options.

## See Also

- [`Sigmoid`](Sigmoid.md) -- the same shape of curve, rescaled to `(0, 1)`
  and centered at `0.5`.
- [`rnn`](../rnn.md) -- `RNN`'s default nonlinearity is `"tanh"`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
