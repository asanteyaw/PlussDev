# `pluss::ai::nn::elu`

Stateless ELU; what Elu's forward calls internally. Declared directly in
`pluss::ai::nn`, unlike the rest of this family (re-exported via
`nn_func.h`).

## Signature

```cpp
Series nn::elu(const Series& x, double alpha = 1.0);
```

## What It Does

Computes `x > 0 ? x : alpha * (exp(x) - 1)` elementwise -- exactly
`x.elu(alpha)`, and exactly what
[`Elu::forward`](../activation/Elu.md#methods) calls internally. Reach
for this directly when the computation is needed inline without
constructing an [`Elu`](../activation/Elu.md) instance; reach for `Elu`
itself when the activation needs to be held and composed as a named
stage in a [`Schedule`](../module/Schedule.md).

## Parameters

`const Series& x`
: Any shape.

`double alpha`
: The saturation floor for negative inputs -- as `x` grows more
  negative, the result approaches `-alpha` without ever reaching or
  crossing it. Defaults to `1.0`.

## Returns

A `Series` the same shape as `x`.

## Errors

None.

## Practical Examples

Three examples: a hand-verified result cross-checked against the exact
formula, `alpha` scaling the negative-side result proportionally, and
confirming this function computes bit-for-bit the same values
[`Elu::forward`](../activation/Elu.md#methods) does.

### Example 1

Five inputs spanning negative, zero, and positive, cross-checked against
the formula computed independently by hand.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 1.0, 3.0});
  const pluss::Series y = nn::elu(x);

  std::cout << "elu(x, alpha=1.0 default): " << y << "\n";

  auto elu_manual = [](double v, double alpha) { return v > 0.0 ? v : alpha * (std::exp(v) - 1.0); };
  std::cout << "manual: [" << elu_manual(-2.0, 1.0) << ", " << elu_manual(-0.5, 1.0) << ", "
            << elu_manual(0.0, 1.0) << ", " << elu_manual(1.0, 1.0) << ", " << elu_manual(3.0, 1.0) << "]\n";

  return 0;
}
```

Output

```text
elu(x, alpha=1.0 default): pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.86466472, -0.39346934, 0, 1, 3]
manual: [-0.864665, -0.393469, 0, 1, 3]
```

### Example 2

For a fixed negative input, `alpha` scales the result exactly
proportionally -- doubling `alpha` doubles the (negative) output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series x = pluss::make_series({-2.0});

  const pluss::Series default_alpha = nn::elu(x);
  const pluss::Series scaled_alpha = nn::elu(x, 2.0);

  std::cout << "elu(-2, alpha=1.0): " << default_alpha.item<double>() << "\n";
  std::cout << "elu(-2, alpha=2.0): " << scaled_alpha.item<double>() << " -- exactly 2x the alpha=1.0 result for negative inputs\n";

  return 0;
}
```

Output

```text
elu(-2, alpha=1.0): -0.864665
elu(-2, alpha=2.0): -1.72933 -- exactly 2x the alpha=1.0 result for negative inputs
```

### Example 3

A live [`Elu`](../activation/Elu.md) instance's `forward` compared
directly against this function called with the same `alpha` -- identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Elu layer(1.5);
  const pluss::Series x = pluss::make_series({-1.0, 2.0});

  const pluss::Series via_module = layer.forward(x);
  const pluss::Series via_function = nn::elu(x, 1.5);

  std::cout << "Elu::forward(x): " << via_module << "\n";
  std::cout << "nn::elu(x, 1.5): " << via_function << "\n";
  std::cout << "identical: " << std::boolalpha << via_module.allclose(via_function) << "\n";

  return 0;
}
```

Output

```text
Elu::forward(x): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.94818084, 2]
nn::elu(x, 1.5): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.94818084, 2]
identical: true
```

## See Also

- [`Elu`](../activation/Elu.md) -- the stateful `Module` wrapper this
  function powers.
- [`fusion`](fusion.md) / [`dropout`](dropout.md) -- the other stateless
  building blocks in this family.
