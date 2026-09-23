# `pluss::ai::nn::Sigmoid`

The logistic sigmoid, applied elementwise: `1 / (1 + exp(-x))`, squashing
any real input into the open interval `(0, 1)`. Commonly used as a final
layer for binary classification (interpreting the output as a probability)
or inside gating mechanisms (an `LSTM`'s own gates use it internally). For
loss computation against a binary target, prefer computing logits and
calling [`losses::BCEWithLogits`](../losses/BCEWithLogits.md) directly
rather than applying `Sigmoid` and then a separate binary cross-entropy on
the result -- the fused form is numerically more stable.

## Signature

```cpp
nn::Sigmoid::Sigmoid();
```

## Construction

`Sigmoid` takes no options -- default-construct it and use it directly, or
add it straight into a [`Schedule`](../module/Schedule.md) with
[`then`](../module/Schedule/then.md).

## State

None. `Sigmoid` has no parameters and no configuration; every instance
behaves identically.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.sigmoid()`: elementwise `1 / (1 + exp(-x))`. Never throws.

## Practical Examples

Two examples: the squashing behavior at the extremes and at `0`, and the
symmetry `sigmoid(-x) == 1 - sigmoid(x)`.

### Example 1

Large-magnitude inputs saturate toward the interval's open bounds; `0` maps
to exactly `0.5`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({-10.0, 0.0, 10.0}, {3}, f64);

  nn::Sigmoid sigmoid;
  const pluss::Series y = sigmoid.forward(x);

  std::cout << "x = " << x << "\n";
  std::cout << "sigmoid(x) = " << y << "\n";

  return 0;
}
```

Output

```text
x = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-10, 0, 10]
sigmoid(x) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4.5397869e-05, 0.5, 0.9999546]
```

### Example 2

`sigmoid(x) + sigmoid(-x) == 1` for any `x` -- the curve is symmetric
around `(0, 0.5)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  nn::Sigmoid sigmoid;

  const pluss::Series x = pluss::make_series({2.0}, {1}, f64);
  const pluss::Series neg_x = pluss::make_series({-2.0}, {1}, f64);

  const pluss::Series s_x = sigmoid.forward(x);
  const pluss::Series s_neg_x = sigmoid.forward(neg_x);
  const pluss::Series one = pluss::Series::full({1}, 1.0, pluss::ScalarType::Float64);

  std::cout << "sigmoid(2)  = " << s_x << "\n";
  std::cout << "sigmoid(-2) = " << s_neg_x << "\n";
  std::cout << "sigmoid(2) + sigmoid(-2) == 1: " << std::boolalpha
            << s_x.add(s_neg_x).allclose(one) << "\n";

  return 0;
}
```

Output

```text
sigmoid(2)  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.88079708]
sigmoid(-2) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.11920292]
sigmoid(2) + sigmoid(-2) == 1: true
```

## Changelog

- **0.1** — the base elementwise logistic sigmoid, no options.

## See Also

- [`Tanh`](Tanh.md) -- the same shape, rescaled to `(-1, 1)` and centered at
  `0`.
- [`losses::BCEWithLogits`](../losses/BCEWithLogits.md) -- the numerically
  stable fused form for binary classification loss.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
