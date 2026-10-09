# `pluss::ai::nn::losses`

`losses` owns the stateless loss functions -- plain functions, not
`Module`s, each comparing a prediction (or logits, or log-probabilities)
against a target and returning a loss value. Five regression losses
([`MSE`](losses/MSE.md), [`RelativeMSE`](losses/RelativeMSE.md),
[`Huber`](losses/Huber.md), [`L1`](losses/L1.md),
[`SmoothL1`](losses/SmoothL1.md)) and three classification losses
([`BCEWithLogits`](losses/BCEWithLogits.md), [`NLL`](losses/NLL.md),
[`CrossEntropy`](losses/CrossEntropy.md)).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`MSE`](losses/MSE.md) | mean squared error |
| [`RelativeMSE`](losses/RelativeMSE.md) | squared error scaled by a caller-supplied divisor |
| [`Huber`](losses/Huber.md) | quadratic near zero, linear beyond `delta` -- outlier-resistant |
| [`L1`](losses/L1.md) | mean absolute error -- linear everywhere, the most outlier-resistant regression loss here |
| [`SmoothL1`](losses/SmoothL1.md) | quadratic near zero, linear beyond `beta`; a numerically different formula from `Huber` despite the similar shape |
| [`BCEWithLogits`](losses/BCEWithLogits.md) | binary cross-entropy computed directly from logits |
| [`NLL`](losses/NLL.md) | negative log-likelihood from log-probabilities and class-index targets |
| [`CrossEntropy`](losses/CrossEntropy.md) | softmax cross-entropy from logits, class-index or soft-probability targets, with label smoothing |

## How To Call It

Compute mean squared error on a three-value vector, then cross-entropy for one row of logits against class zero. The program prints each loss.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  const pluss::Series prediction = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series target = pluss::make_series({1.5, 2.0, 2.0});
  const pluss::Series loss = nn::losses::MSE(prediction, target);
  std::cout << "mse = " << loss.item<double>() << "\n";
  const pluss::Series logits = pluss::make_series({2.0, 1.0, 0.1}, {1, 3});
  const pluss::Series class_target = pluss::make_series({0LL}, {1}).to(pluss::ScalarType::Int64);
  const pluss::Series ce = nn::losses::CrossEntropy(logits, class_target);
  std::cout << "cross entropy = " << ce.item<double>() << "\n";
  return 0;
}
```

Output

```text
mse = 0.416667
cross entropy = 0.41703
```

Every loss here is a plain function -- there is no type to construct, no
`->` indirection, and no `parameters()`. Every loss's third argument is
its own `Options` struct, always defaulted, so the common case omits it
entirely (as shown for `MSE` and `CrossEntropy` above); the shared
`reduction` field every one of them has means the same thing everywhere
-- see [`Introduction`](losses/Introduction.md#the-shared-reduction-mode).

## Related Pages

- [`attention`](attention.md) / [`rnn`](rnn.md) -- the layer families
  whose output a loss in this family is typically called on.
- [`module`](module.md) -- the `Module` contract most of the rest of
  this reference implements; none of the losses in this family do.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

losses/Introduction
losses/MSE
losses/MSEOptions
losses/RelativeMSE
losses/RelativeMSEOptions
losses/Huber
losses/HuberOptions
losses/L1
losses/L1Options
losses/SmoothL1
losses/SmoothL1Options
losses/BCEWithLogits
losses/BCEWithLogitsOptions
losses/NLL
losses/NLLOptions
losses/CrossEntropy
losses/CrossEntropyOptions
```
<!-- apiref:toctree:end -->
