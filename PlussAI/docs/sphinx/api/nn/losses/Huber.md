# `pluss::ai::nn::losses::Huber`

Quadratic near zero error, linear beyond a delta threshold -- robust to
outliers.

## Signature

```cpp
Series nn::losses::Huber(const Series& prediction, const Series& target,
                          const nn::losses::HuberOptions& options = {});
```

## What It Does

Computes, elementwise, `0.5 * error^2` when `|error| <= options.delta`,
or `options.delta * |error| - 0.5 * options.delta^2` when `|error| >
options.delta` (where `error = prediction - target`), then reduces the
result per `options.reduction`. The two pieces meet exactly at `|error| =
delta` -- both the value and the slope are continuous there, which is
what makes this a genuine single loss rather than two losses stitched
together. The practical effect: an ordinary, small error is penalized the
same as [`MSE`](MSE.md) would (quadratically), but a single large
outlier's contribution grows only linearly rather than quadratically, so
it cannot dominate a batch's mean loss the way a squared error can. See
Example 3.

## Parameters

`const Series& prediction`
: The model's output. Any shape; must match `target`'s shape exactly.

`const Series& target`
: The ground-truth value. Same shape as `prediction`.

`const HuberOptions& options`
: See [`HuberOptions`](HuberOptions.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a `Series`
the same shape as `prediction` when `None`.

## Errors

- `prediction`/`target` undefined, mismatched shapes, a non-float dtype,
  or different platforms -- throws.
- `options.delta` non-positive or non-finite -- throws.

## Practical Examples

Three examples: a hand-verified per-element result with one error inside
and one outside the `delta` threshold, `delta` shifting where that
threshold falls, and a direct comparison against `MSE` on a batch with
one outlier, showing exactly why this loss exists.

### Example 1

An error of `0.5` (inside the default `delta=1`) gets the plain
quadratic formula; an error of `2.0` (outside it) gets the linear one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // errors: 0.5 (inside delta=1, quadratic region) and 2.0 (outside, linear region)
  const pluss::Series prediction = pluss::make_series({0.5, 2.0});
  const pluss::Series target = pluss::make_series({0.0, 0.0});

  nn::losses::HuberOptions opts{};
  opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::Huber(prediction, target, opts);

  std::cout << "per-element Huber loss (delta=1 default): " << per_element << "\n";
  std::cout << "element 0 (|err|=0.5 <= delta): 0.5*0.5^2 = 0.125 (plain quadratic)\n";
  std::cout << "element 1 (|err|=2.0 > delta):  0.5*1^2 + 1*(2.0-1) = 0.5+1 = 1.5 (linear beyond)\n";

  return 0;
}
```

Output

```text
per-element Huber loss (delta=1 default): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.125, 1.5]
element 0 (|err|=0.5 <= delta): 0.5*0.5^2 = 0.125 (plain quadratic)
element 1 (|err|=2.0 > delta):  0.5*1^2 + 1*(2.0-1) = 0.5+1 = 1.5 (linear beyond)
```

### Example 2

The same `|error|=3` value, once with a `delta` narrower than the error
(linear regime) and once with a `delta` wider than it (quadratic
regime).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({3.0});
  const pluss::Series target = pluss::make_series({0.0});  // |err| = 3.0 always

  nn::losses::HuberOptions small_delta{};
  small_delta.delta = 1.0;
  const pluss::Series loss_small = nn::losses::Huber(prediction, target, small_delta);
  std::cout << "delta=1: " << loss_small.item<double>() << " -- (|err|=3 > delta): 0.5*1 + 1*(3-1) = 0.5+2 = 2.5\n";

  nn::losses::HuberOptions large_delta{};
  large_delta.delta = 5.0;
  const pluss::Series loss_large = nn::losses::Huber(prediction, target, large_delta);
  std::cout << "delta=5: " << loss_large.item<double>() << " -- (|err|=3 <= delta): plain quadratic 0.5*3^2 = 4.5\n";

  return 0;
}
```

Output

```text
delta=1: 2.5 -- (|err|=3 > delta): 0.5*1 + 1*(3-1) = 0.5+2 = 2.5
delta=5: 4.5 -- (|err|=3 <= delta): plain quadratic 0.5*3^2 = 4.5
```

### Example 3

Four ordinary points plus one severe outlier (an error of `100`): `MSE`
squares that outlier's error, letting it dominate the batch's mean loss
by roughly `100x` over `Huber`, which caps its per-element contribution
to linear growth beyond `delta`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // 4 ordinary points with small errors, and 1 outlier with a large error
  const pluss::Series prediction = pluss::make_series({1.0, 2.0, 3.0, 4.0, 100.0});
  const pluss::Series target = pluss::make_series({1.1, 2.1, 3.1, 4.1, 0.0});

  const pluss::Series mse = nn::losses::MSE(prediction, target);
  const pluss::Series huber = nn::losses::Huber(prediction, target);

  std::cout << "MSE (squares the 100-unit outlier error, dominates the mean): " << mse.item<double>() << "\n";
  std::cout << "Huber (delta=1, grows only linearly past the outlier):        " << huber.item<double>() << "\n";
  std::cout << "MSE / Huber ratio: " << (mse.item<double>() / huber.item<double>()) << "x\n";

  return 0;
}
```

Output

```text
MSE (squares the 100-unit outlier error, dominates the mean): 2000.01
Huber (delta=1, grows only linearly past the outlier):        19.904
MSE / Huber ratio: 100.483x
```

## See Also

- [`HuberOptions`](HuberOptions.md) -- the options struct.
- [`SmoothL1`](SmoothL1.md) -- a related but numerically different
  quadratic/linear loss.
- [`MSE`](MSE.md) -- the purely quadratic counterpart this improves on
  for outlier robustness.
- [`L1`](L1.md) -- the purely linear alternative, `Huber`'s limit as
  `delta -> 0`.
