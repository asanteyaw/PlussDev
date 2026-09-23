# `pluss::ai::nn::Softmax`

Normalizes a `Series` into a probability distribution along one axis:
exponentiate every entry, then divide by the sum along `dim`. The result is
non-negative and sums to exactly `1` along that axis -- the standard way to
turn a layer of raw class logits into per-class probabilities.

## Signature

```cpp
explicit nn::Softmax::Softmax(nn::SoftmaxOptions options = {});
```

## Construction

Default-construct to normalize over the last axis (`dim=-1`), or pass a
[`SoftmaxOptions`](SoftmaxOptions.md) to normalize over a different axis.

## State

`options()` (mutable and `const` overloads) returns the
[`SoftmaxOptions`](SoftmaxOptions.md) this instance was constructed with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.softmax(options().dim)`. Never throws for a shape where
  `dim` is a valid axis.

## Practical Examples

Three examples: normalizing a row of logits into a distribution, choosing a
different axis on a matrix, and confirming `Softmax` runs on the MPS
backend.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series logits = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  nn::Softmax softmax;  // dim=-1 (default)
  const pluss::Series probs = softmax.forward(logits);

  std::cout << "logits = " << logits << "\n";
  std::cout << "softmax(logits) = " << probs << "\n";
  std::cout << "sums to 1: " << std::boolalpha << probs.sum().allclose(pluss::Series::full({}, 1.0, pluss::ScalarType::Float64)) << "\n";

  return 0;
}
```

Output

```text
logits = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 2, 3]]
softmax(logits) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.090030573, 0.24472847, 0.66524096]]
sums to 1: true
```

### Example 2

On a matrix, `dim` picks which axis is normalized: `dim=-1` normalizes each
row into its own distribution, `dim=0` normalizes each column instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series logits = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3}, f64);

  nn::SoftmaxOptions last_axis{};
  last_axis.dim = -1;
  nn::Softmax over_rows(last_axis);

  nn::SoftmaxOptions first_axis{};
  first_axis.dim = 0;
  nn::Softmax over_cols(first_axis);

  const Series row_probs = over_rows.forward(logits);
  const Series col_probs = over_cols.forward(logits);

  std::cout << "logits = " << logits << "\n";
  std::cout << "dim=-1 (each row sums to 1): " << row_probs << "\n";
  std::cout << "dim=0 (each column sums to 1): " << col_probs << "\n";

  return 0;
}
```

Output

```text
logits = pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6]]
dim=-1 (each row sums to 1): pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.090030573, 0.24472847, 0.66524096],
  [0.090030573, 0.24472847, 0.66524096]]
dim=0 (each column sums to 1): pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.047425873, 0.047425873, 0.047425873],
  [0.95257413, 0.95257413, 0.95257413]]
```

### Example 3

`Softmax` runs on the MPS backend the same way it does on CPU -- move the
input to `pMPS` first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series logits = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}).to(ScalarType::Float32).to(pluss::pMPS);

  nn::Softmax softmax;
  try {
    const Series y = softmax.forward(logits);
    std::cout << "softmax on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "softmax on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
softmax on MPS: OK, platform=2
```

## Changelog

- **0.1** — the softmax normalization, `dim` option.

## See Also

- [`SoftmaxOptions`](SoftmaxOptions.md) -- the options struct.
- [`LogSoftmax`](LogSoftmax.md) -- the numerically stabler log-domain form,
  usually preferred right before a loss like cross-entropy.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

SoftmaxOptions
```
<!-- apiref:toctree:end -->
