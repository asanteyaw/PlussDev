# `pluss::ai::nn::losses::SmoothL1`

Quadratic near zero error, linear beyond a beta threshold; reduces to L1
at beta = 0.

## Signature

```cpp
Series nn::losses::SmoothL1(const Series& prediction, const Series& target,
                             const nn::losses::SmoothL1Options& options = {});
```

## What It Does

Computes, elementwise, `error^2 / (2 * options.beta)` when `|error| <
options.beta`, or `|error| - 0.5 * options.beta` when `|error| >=
options.beta` (where `error = prediction - target`), then reduces the
result per `options.reduction`. The two pieces meet continuously (both
value and slope) at `|error| = beta`, the same property
[`Huber`](Huber.md) has -- but the two formulas are genuinely different,
not two names for the same loss; see Example 2. At `options.beta = 0`
exactly, this function delegates directly to [`L1`](L1.md) instead of
evaluating the formula above (which would divide by zero at `beta = 0`).

## Parameters

`const Series& prediction`
: The model's output. Any shape; must match `target`'s shape exactly.

`const Series& target`
: The ground-truth value. Same shape as `prediction`.

`const SmoothL1Options& options`
: See [`SmoothL1Options`](SmoothL1Options.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a `Series`
the same shape as `prediction` when `None`.

## Errors

- `prediction`/`target` undefined, mismatched shapes, a non-float dtype,
  or different platforms -- throws.
- `options.beta` negative or non-finite -- throws (`0` itself is valid,
  and takes the `L1` shortcut).

## Practical Examples

Three examples: a hand-verified per-element result with one error inside
and one outside the `beta` threshold, a direct numeric comparison against
`Huber` at a matching threshold value proving the two formulas differ,
and confirming the quadratic and linear pieces meet continuously right
at the transition.

### Example 1

An error of `0.5` (inside the default `beta=1`) gets the quadratic
formula; an error of `2.0` (outside it) gets the linear one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // errors: 0.5 (inside beta=1, quadratic region) and 2.0 (outside, linear region)
  const pluss::Series prediction = pluss::make_series({0.5, 2.0});
  const pluss::Series target = pluss::make_series({0.0, 0.0});

  nn::losses::SmoothL1Options opts{};
  opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::SmoothL1(prediction, target, opts);

  std::cout << "per-element SmoothL1 loss (beta=1 default): " << per_element << "\n";
  std::cout << "element 0 (|err|=0.5 < beta): 0.5^2/(2*1) = 0.125 (quadratic)\n";
  std::cout << "element 1 (|err|=2.0 >= beta): 2.0-0.5*1 = 1.5 (linear beyond)\n";

  return 0;
}
```

Output

```text
per-element SmoothL1 loss (beta=1 default): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.125, 1.5]
element 0 (|err|=0.5 < beta): 0.5^2/(2*1) = 0.125 (quadratic)
element 1 (|err|=2.0 >= beta): 2.0-0.5*1 = 1.5 (linear beyond)
```

### Example 2

`SmoothL1(beta=2)` and `Huber(delta=2)`, evaluated on the exact same
errors -- despite the matching threshold value, the results differ.
`SmoothL1`'s quadratic term (`error^2 / (2*beta)`) is scaled down by
`1/beta`; `Huber`'s (`0.5 * error^2`) is not, so `Huber` reports a larger
value at every point here.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({1.0, 3.0});
  const pluss::Series target = pluss::make_series({0.0, 0.0});  // errors: 1.0 (inside), 3.0 (outside)

  nn::losses::SmoothL1Options sl1_opts{};
  sl1_opts.beta = 2.0;
  sl1_opts.reduction = nn::LossReduction::None;
  const pluss::Series smooth_l1 = nn::losses::SmoothL1(prediction, target, sl1_opts);

  nn::losses::HuberOptions huber_opts{};
  huber_opts.delta = 2.0;
  huber_opts.reduction = nn::LossReduction::None;
  const pluss::Series huber = nn::losses::Huber(prediction, target, huber_opts);

  std::cout << "SmoothL1(beta=2):  " << smooth_l1 << "\n";
  std::cout << "Huber(delta=2):    " << huber << "\n";
  std::cout << "these are NOT the same loss, even at matching thresholds -- "
            << "SmoothL1's quadratic term is scaled down by 1/beta, Huber's is not\n";

  return 0;
}
```

Output

```text
SmoothL1(beta=2):  pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.25, 2]
Huber(delta=2):    pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 4]
these are NOT the same loss, even at matching thresholds -- SmoothL1's quadratic term is scaled down by 1/beta, Huber's is not
```

### Example 3

Errors just below, exactly at, and just above `beta` -- the quadratic
and linear pieces produce nearly identical values right at the
transition, confirming there is no discontinuity to worry about when an
error happens to land near the threshold.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // errors just below, at, and just above beta=1 -- the quadratic and
  // linear pieces must meet exactly at the transition, with no jump
  const pluss::Series prediction = pluss::make_series({0.999, 1.0, 1.001});
  const pluss::Series target = pluss::make_series({0.0, 0.0, 0.0});

  nn::losses::SmoothL1Options opts{};
  opts.beta = 1.0;
  opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::SmoothL1(prediction, target, opts);

  std::cout << "just below beta, at beta, just above beta: " << per_element << "\n";
  std::cout << "the three values are nearly identical -- the quadratic and linear\n";
  std::cout << "pieces meet continuously at |error| = beta, with no discontinuity\n";

  return 0;
}
```

Output

```text
just below beta, at beta, just above beta: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.4990005, 0.5, 0.501]
the three values are nearly identical -- the quadratic and linear
pieces meet continuously at |error| = beta, with no discontinuity
```

## See Also

- [`SmoothL1Options`](SmoothL1Options.md) -- the options struct.
- [`L1`](L1.md) -- what this reduces to exactly at `beta = 0`.
- [`Huber`](Huber.md) -- a related but numerically different
  quadratic/linear loss.
- [`MSE`](MSE.md) -- the purely quadratic alternative.
