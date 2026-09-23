# `pluss::ai::nn::LogSoftmax`

The log of [`Softmax`](Softmax.md), applied along one axis:
`log(exp(x) / sum(exp(x)))` -- computed as a single numerically stable
operation rather than `softmax` followed by a separate `log` call, so it
avoids the underflow that can happen when a very small probability gets
logged. This is the form most classification losses (cross-entropy chief
among them) expect to consume directly, in preference to raw `softmax`
output.

## Signature

```cpp
explicit nn::LogSoftmax::LogSoftmax(nn::LogSoftmaxOptions options = {});
```

## Construction

Default-construct to normalize over the last axis (`dim=-1`), or pass a
[`LogSoftmaxOptions`](LogSoftmaxOptions.md) to normalize over a different
axis.

## State

`options()` (mutable and `const` overloads) returns the
[`LogSoftmaxOptions`](LogSoftmaxOptions.md) this instance was constructed
with.

## Methods

`forward(input)` / `operator()(input)` -- `Series`
: Returns `input.log_softmax(options().dim)`. Never throws for a shape
  where `dim` is a valid axis.

## Practical Examples

Three examples: the basic log-probability output, confirming it agrees with
`softmax(...).log()` on well-scaled input, and confirming `LogSoftmax` runs
on the MPS backend.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series logits = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  nn::LogSoftmax log_softmax;  // dim=-1 (default)
  const pluss::Series y = log_softmax.forward(logits);

  std::cout << "logits = " << logits << "\n";
  std::cout << "log_softmax(logits) = " << y << "\n";

  return 0;
}
```

Output

```text
logits = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 2, 3]]
log_softmax(logits) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-2.407606, -1.407606, -0.40760596]]
```

Every entry is negative (a log-probability, since the underlying
probabilities are all below `1`), and `log_softmax(logits).exp()` would
recover the same probabilities `softmax(logits)` produces directly.

### Example 2

On well-scaled input where no underflow occurs, `log_softmax(x)` and
`softmax(x).log()` agree exactly -- `log_softmax` exists for numerical
stability at extreme input magnitudes, not for a different mathematical
result.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series logits = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}, f64);

  nn::LogSoftmax log_softmax;
  nn::Softmax softmax;

  const Series via_log_softmax = log_softmax.forward(logits);
  const Series via_log_of_softmax = softmax.forward(logits).log();

  std::cout << "log_softmax(logits)     = " << via_log_softmax << "\n";
  std::cout << "softmax(logits).log()   = " << via_log_of_softmax << "\n";
  std::cout << "same values: " << std::boolalpha << via_log_softmax.allclose(via_log_of_softmax) << "\n";

  return 0;
}
```

Output

```text
log_softmax(logits)     = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-2.407606, -1.407606, -0.40760596]]
softmax(logits).log()   = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-2.407606, -1.407606, -0.40760596]]
same values: true
```

### Example 3

`LogSoftmax` runs on the MPS backend the same way it does on CPU -- move
the input to `pMPS` first.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;
  using pluss::ScalarType;

  const Series logits = pluss::make_series({1.0, 2.0, 3.0}, {1, 3}).to(ScalarType::Float32).to(pluss::pMPS);

  nn::LogSoftmax log_softmax;
  try {
    const Series y = log_softmax.forward(logits);
    std::cout << "log_softmax on MPS: OK, platform=" << static_cast<int>(y.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "log_softmax on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
log_softmax on MPS: OK, platform=2
```

## Changelog

- **0.1** — the log-softmax normalization, `dim` option.

## See Also

- [`LogSoftmaxOptions`](LogSoftmaxOptions.md) -- the options struct.
- [`Softmax`](Softmax.md) -- the plain-probability form this is built from.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LogSoftmaxOptions
```
<!-- apiref:toctree:end -->
