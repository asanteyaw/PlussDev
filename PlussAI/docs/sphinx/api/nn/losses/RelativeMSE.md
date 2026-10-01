# `pluss::ai::nn::losses::RelativeMSE`

Squared error scaled by a caller-supplied divisor, for scale-invariant
regression targets.

## Signature

```cpp
Series nn::losses::RelativeMSE(const Series& prediction, const Series& target,
                                const Series& divisor,
                                const nn::losses::RelativeMSEOptions& options = {});
```

## What It Does

Computes `(prediction - target)^2 / denom` elementwise, then reduces it
according to `options.reduction` (see
[`RelativeMSEOptions`](RelativeMSEOptions.md)), where `denom` is
`divisor` (optionally `|divisor|`, per `options.use_absolute_divisor`)
floored up to `options.epsilon`. `divisor` is caller-supplied rather than
derived automatically -- ordinarily `target` itself, or `target`'s own
magnitude, when the intent is a relative (percentage-style) error that
does not penalize large-magnitude targets any harder than small ones,
the way plain [`MSE`](MSE.md) does. `prediction`, `target`, and `divisor`
must all share a platform; `divisor` need not share `prediction`/
`target`'s exact shape, only be broadcastable against the squared-error
term.

## Parameters

`const Series& prediction`
: The model's output. Any shape; must match `target`'s shape exactly.

`const Series& target`
: The ground-truth value. Same shape as `prediction`.

`const Series& divisor`
: What each element's squared error is divided by. Commonly `target`
  itself (see Example 1), but any caller-supplied `Series` -- there is no
  requirement that it be derived from `prediction`/`target` at all.

`const RelativeMSEOptions& options`
: See [`RelativeMSEOptions`](RelativeMSEOptions.md).

## Returns

A scalar `Series` when `options.reduction` is `Mean` or `Sum`; a `Series`
matching the squared-error term's broadcast shape when `None`.

## Errors

- `prediction`/`target`/`divisor` undefined, a non-float dtype, or
  different platforms -- throws.
- `options.epsilon` non-positive or non-finite -- throws.

## Practical Examples

Three examples: dividing by the target's own magnitude for a
scale-invariant error, the `use_absolute_divisor` toggle's effect on a
negative divisor, and the `epsilon` floor protecting a zero divisor from
producing an undefined result.

### Example 1

Dividing by `target` itself makes a `10`-unit error on a target of `100`
contribute far less than a `2`-unit error on a target of `12` -- the
defining behavior a relative error metric provides that plain
[`MSE`](MSE.md) does not.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({10.0, 100.0});
  const pluss::Series target = pluss::make_series({12.0, 90.0});
  const pluss::Series divisor = pluss::make_series({12.0, 90.0});  // scale error relative to target's own magnitude

  const pluss::Series loss = nn::losses::RelativeMSE(prediction, target, divisor);
  std::cout << "RelativeMSE (mean): " << loss.item<double>() << "\n";

  // manual check: (10-12)^2/12 = 0.33333, (100-90)^2/90 = 1.11111, mean = 0.72222
  const pluss::Series manual = prediction.sub(target).square().div(divisor).mean();
  std::cout << "manual mean((pred-target)^2/divisor): " << manual.item<double>() << "\n";

  return 0;
}
```

Output

```text
RelativeMSE (mean): 0.722222
manual mean((pred-target)^2/divisor): 0.722222
```

### Example 2

`use_absolute_divisor` decides whether a negative `divisor` is treated as
a magnitude first. With it `true` (the default), `-4` becomes `4` before
dividing -- an ordinary result. With it `false`, the raw `-4` is passed
to `clamp_min`, which only ever raises a value -- so it floors up to the
tiny **positive** `epsilon`, not down to a negative result, producing an
artificially enormous loss instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({10.0});
  const pluss::Series target = pluss::make_series({12.0});
  const pluss::Series negative_divisor = pluss::make_series({-4.0});

  // use_absolute_divisor=true (default): |−4| = 4, ordinary result
  const pluss::Series loss_abs = nn::losses::RelativeMSE(prediction, target, negative_divisor);
  std::cout << "use_absolute_divisor=true:  " << loss_abs.item<double>() << " -- (10-12)^2/|−4| = 4/4 = 1\n";

  // use_absolute_divisor=false: the raw negative divisor is clamped up to
  // epsilon (a tiny POSITIVE floor), not left negative -- dividing by a
  // near-zero positive number, not the actual -4
  nn::losses::RelativeMSEOptions opts{};
  opts.use_absolute_divisor = false;
  const pluss::Series loss_raw = nn::losses::RelativeMSE(prediction, target, negative_divisor, opts);
  std::cout << "use_absolute_divisor=false: " << loss_raw.item<double>()
            << " -- clamp_min floors -4 up to epsilon (1e-12), producing a huge loss, not a negative one\n";

  return 0;
}
```

Output

```text
use_absolute_divisor=true:  1 -- (10-12)^2/|−4| = 4/4 = 1
use_absolute_divisor=false: 4e+12 -- clamp_min floors -4 up to epsilon (1e-12), producing a huge loss, not a negative one
```

### Example 3

A `divisor` of exactly `0` (a target that happens to be zero, when
dividing by the target itself) would ordinarily be undefined -- `epsilon`
floors it up first, so the result is a very large finite number, never a
`NaN` or `inf`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series prediction = pluss::make_series({1.0, 5.0});
  const pluss::Series target = pluss::make_series({0.0, 5.0});     // first target is exactly zero
  const pluss::Series divisor = pluss::make_series({0.0, 5.0});    // dividing by zero would be undefined

  nn::losses::RelativeMSEOptions opts{};
  opts.reduction = nn::LossReduction::None;
  const pluss::Series per_element = nn::losses::RelativeMSE(prediction, target, divisor, opts);

  std::cout << "per-element relative squared error: " << per_element << "\n";
  std::cout << "element 0: divisor=0 floored to epsilon (1e-12), so (1-0)^2/1e-12 is enormous, not a NaN/inf\n";
  std::cout << "element 1: divisor=5 (unaffected), ordinary (5-5)^2/5 = 0\n";

  return 0;
}
```

Output

```text
per-element relative squared error: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1e+12, 0]
element 0: divisor=0 floored to epsilon (1e-12), so (1-0)^2/1e-12 is enormous, not a NaN/inf
element 1: divisor=5 (unaffected), ordinary (5-5)^2/5 = 0
```

## See Also

- [`RelativeMSEOptions`](RelativeMSEOptions.md) -- the options struct.
- [`MSE`](MSE.md) -- the unscaled counterpart this generalizes.
