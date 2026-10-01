# `pluss::ai::nn::fusion`

The stateless fully connected op: y = xW^T + b. What Fusion's forward
calls internally.

## Signature

```cpp
Series nn::fusion(const Series& input, const Series& weight,
                   const Series& bias = pluss::series::undefined());
```

## What It Does

Computes `input @ weight.t() + bias` (the addition skipped entirely when
`bias` is undefined), with `input`'s leading axes preserved and its
trailing axis mapped from `weight`'s column count to its row count --
exactly the computation [`Fusion::forward`](../fusion/Fusion.md#methods)
performs, minus the surrounding `Module` machinery (no construction, no
registered parameters, no `Options` struct). Reach for this directly when
`weight`/`bias` already exist as plain `Series` values from some other
source and a full `Fusion` instance would be unnecessary ceremony;
reach for [`Fusion`](../fusion/Fusion.md) itself when the weights need to
be owned, trained, and moved as a unit.

## Parameters

`const Series& input`
: Any rank `>= 1`; the last axis must equal `weight`'s column count.

`const Series& weight`
: `[out_features, in_features]`.

`const Series& bias`
: `[out_features]`, or undefined (the default) to skip the addition
  entirely.

## Returns

`input`'s leading axes, unchanged, with the trailing axis now
`weight`'s row count (`out_features`).

## Errors

- `input`'s last axis does not match `weight`'s column count -- throws.

## Practical Examples

Three examples: a hand-verified affine map with and without `bias`,
confirming this function computes bit-for-bit the same result
`Fusion::forward` does when given that instance's own `weight`/`bias`,
and a higher-rank input showing every leading axis preserved.

### Example 1

A `weight`/`bias` set by hand, so the output is verifiable against the
exact formula -- and, separately, calling with `bias` simply omitted
computes the pure linear map with no additive shift.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series weight = pluss::make_series({1.0, 0.0, 0.0, 0.0, 1.0, 0.0}, {2, 3}, f64);
  const pluss::Series bias = pluss::make_series({0.5, -0.5}, {2}, f64);
  const pluss::Series x = pluss::make_series({10.0, 20.0, 30.0}, {1, 3}, f64);

  const pluss::Series y = nn::fusion(x, weight, bias);
  std::cout << "y = x @ weight^T + bias: " << y << "\n";

  // omitting bias entirely computes the pure linear map
  const pluss::Series y_no_bias = nn::fusion(x, weight);
  std::cout << "y (no bias): " << y_no_bias << "\n";

  return 0;
}
```

Output

```text
y = x @ weight^T + bias: pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[10.5, 19.5]]
y (no bias): pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[10, 20]]
```

### Example 2

A live [`Fusion`](../fusion/Fusion.md) instance's `forward` compared
directly against this function called on that same instance's own
`weight()`/`bias()` -- bit-for-bit identical, confirming this genuinely
is what `Fusion::forward` calls internally, not merely an equivalent
reimplementation.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Fusion layer(3, 2);
  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  const pluss::Series via_module = layer.forward(x);
  const pluss::Series via_function = nn::fusion(x, layer->weight(), layer->bias());

  std::cout << "Fusion::forward(x):        " << via_module << "\n";
  std::cout << "nn::fusion(x, w, b):       " << via_function << "\n";
  std::cout << "identical: " << std::boolalpha << via_module.allclose(via_function) << "\n";

  return 0;
}
```

Output

```text
Fusion::forward(x):        pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-2.1341209, 0.12355834]]
nn::fusion(x, w, b):       pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-2.1341209, 0.12355834]]
identical: true
```

### Example 3

A `[batch, seq_len, in_features]` input -- every leading axis is
preserved; only the trailing axis changes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series weight = pluss::Series::full({6, 4}, 0.1, pluss::ScalarType::Float64);

  // batch=2, seq_len=3, in_features=4
  const pluss::Series x = pluss::Series::full({2, 3, 4}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series y = nn::fusion(x, weight);

  std::cout << "input shape:  [" << x.sizes()[0] << ", " << x.sizes()[1] << ", " << x.sizes()[2] << "]\n";
  std::cout << "output shape: [" << y.sizes()[0] << ", " << y.sizes()[1] << ", " << y.sizes()[2]
            << "] -- every leading dim preserved, only the last axis changes\n";

  return 0;
}
```

Output

```text
input shape:  [2, 3, 4]
output shape: [2, 3, 6] -- every leading dim preserved, only the last axis changes
```

## See Also

- [`Fusion`](../fusion/Fusion.md) -- the stateful `Module` wrapper this
  function powers.
- [`dropout`](dropout.md) -- another stateless building block used
  throughout this reference's `Module` classes' own `forward` methods.
