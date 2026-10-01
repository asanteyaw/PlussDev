# `pluss::ai::nn::dropout`

Stateless elementwise dropout, active only when training is true.

## Signature

```cpp
Series nn::dropout(const Series& x, double p, bool training = true, std::uint64_t seed = 0);
```

## What It Does

Independently zeroes each element of `x` with probability `p`, and
scales every surviving element by `1 / (1 - p)` -- "inverted dropout",
the same convention every dropout call in this reference uses, so the
output's expected value matches `x`'s own value regardless of `p`. Only
takes effect when `training == true`; with `training == false`, `x` is
returned completely unchanged, dropped nor scaled. This is the exact
function every `dropout` field in this reference's `Options` structs
(`FusionOptions` has none, but `TransformerEncoderLayerOptions::dropout`,
`RNNOptions::dropout`, and others) is threaded through to internally.

## Parameters

`const Series& x`
: Any shape.

`double p`
: The probability of zeroing each element independently. Must be in
  `[0, 1]`. `0.0` is a no-op regardless of `training`/`seed`.

`bool training`
: When `false`, `x` passes through completely unchanged -- no
  computation beyond the check itself. Defaults to `true`.

`std::uint64_t seed`
: Which elements are dropped is a deterministic function of `seed` --
  the same `seed` on the same shape always drops the exact same
  elements. Defaults to `0`.

## Returns

A `Series` the same shape as `x`.

## Errors

- `p` outside `[0, 1]` -- throws.

## Practical Examples

Three examples: confirming the inverted-dropout scaling and the
`training=false` passthrough, `seed` reproducibility, and an empirical
check that the actual drop rate over many elements matches the requested
`p`.

### Example 1

With `p=0.5`, every surviving element reads `2.0` (`1/(1-0.5)`), not
`1.0` -- the scaling that keeps the output's expected value matching the
input's. `training=false` skips dropping entirely, regardless of `p` or
`seed`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series x = pluss::Series::full({10}, 1.0, pluss::ScalarType::Float64);

  const pluss::Series dropped = nn::dropout(x, 0.5, /*training=*/true, /*seed=*/42);
  std::cout << "dropout(x, p=0.5, training=true): " << dropped << "\n";
  std::cout << "surviving elements are scaled up (inverted dropout), not left at 1.0 -- "
               "each survivor reads 1/(1-p)=2.0, so the expected sum stays unchanged\n";

  const pluss::Series unchanged = nn::dropout(x, 0.5, /*training=*/false, /*seed=*/42);
  std::cout << "dropout(x, p=0.5, training=false): " << unchanged << " -- eval mode never drops\n";

  return 0;
}
```

Output

```text
dropout(x, p=0.5, training=true): pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[2, 0, 0, 0, 0, 2, 0, 2, 0, 2]
surviving elements are scaled up (inverted dropout), not left at 1.0 -- each survivor reads 1/(1-p)=2.0, so the expected sum stays unchanged
dropout(x, p=0.5, training=false): pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[1, 1, 1, 1, 1, 1, 1, 1, 1, 1] -- eval mode never drops
```

### Example 2

The same `seed` always reproduces the exact same dropped elements; a
different `seed` reproduces a different (but still deterministic) set.
`p=0.0` is a no-op regardless of `seed`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series x = pluss::Series::full({10}, 1.0, pluss::ScalarType::Float64);

  const pluss::Series a = nn::dropout(x, 0.5, true, 7);
  const pluss::Series b = nn::dropout(x, 0.5, true, 7);
  const pluss::Series c = nn::dropout(x, 0.5, true, 8);

  std::cout << "same seed (7) twice, identical mask: " << std::boolalpha << a.allclose(b) << "\n";
  std::cout << "different seed (8), different mask:  " << !a.allclose(c) << "\n";

  // p=0.0 is a no-op regardless of training/seed
  const pluss::Series untouched = nn::dropout(x, 0.0, true, 123);
  std::cout << "p=0.0: " << untouched << "\n";

  return 0;
}
```

Output

```text
same seed (7) twice, identical mask: true
different seed (8), different mask:  true
p=0.0: pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
```

### Example 3

Over `10000` elements, the empirical drop rate lands close to the
requested `p=0.3` -- confirming `p` genuinely is a per-element
probability, not, say, a fraction rounded to the nearest exact count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series x = pluss::Series::full({10000}, 1.0, pluss::ScalarType::Float64);
  const pluss::Series dropped = nn::dropout(x, 0.3, true, 99);

  const pluss::Series zero_count = dropped.eq(0.0).to(pluss::ScalarType::Float64).sum();
  const double empirical_rate = zero_count.item<double>() / 10000.0;
  std::cout << "requested p=0.3, empirical drop rate over 10000 elements: " << empirical_rate << "\n";

  return 0;
}
```

Output

```text
requested p=0.3, empirical drop rate over 10000 elements: 0.3036
```

## See Also

- [`fusion`](fusion.md) -- another stateless building block used
  throughout this reference's `Module` classes' own `forward` methods.
