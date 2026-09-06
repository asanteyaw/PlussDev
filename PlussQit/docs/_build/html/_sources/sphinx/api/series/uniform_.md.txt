<!-- pluss-hand-authored -->

# Series::uniform_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::uniform_() const
Series& Series::uniform_(double from, double to) const
```

## What It Does

Overwrites every element of the receiver with an independent value drawn
uniformly from the half-open interval `[from, to)`.

With no arguments the interval is `[0, 1)`. Every equal-length sub-interval
is equally likely; the receiver's current values are discarded and play no
role in the draw.

Like [`normal_`](normal_.md), this is the in-place "fill this buffer"
primitive — the receiver supplies the shape, dtype, platform, and storage.
The factory [`rand`](../series_factory/rand.md) is the *create-a-new-tensor*
counterpart.

**Half-open** means `from` can be produced but `to` is (with probability 1)
not. **Equal bounds** are allowed: `uniform_(c, c)` is a well-defined
zero-width distribution and fills every element with `c` — not an error.

### Reproducibility

The draw is deterministic given the process-wide RNG seed. Call
`pluss::rng_seed(seed)` before sampling to fix the exact values; two runs
from the same seed produce `equal` tensors. Each call advances a stream
counter, so back-to-back draws differ unless you reseed.

## Parameters

**from** (`double`): the (inclusive) lower bound. Must be finite.

**to** (`double`): the (exclusive) upper bound. Must be finite and `>= from`.

## Returns

`Series&`
    A reference to the receiver, now holding the samples. The write is
    in-place and preserves storage identity — every alias sees the new
    values.

## Raises

- if the receiver's dtype is not floating-point or complex
- if `from` or `to` is non-finite, or `to < from`

## Gradient

Not differentiable — a random overwrite carries no derivative through the
replaced values or the bounds.

## Examples

Example 1. Default fill from `[0, 1)`. Seeded, so the values are fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({5}, pluss::ScalarType::Float64);
x.uniform_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.8438, 0.1131, 0.2531, 0.2190, 0.5193]
```

Example 2. `uniform_(from, to)` — a large sample stays inside the interval,
with its mean near the midpoint.

```cpp
pluss::rng_seed(3);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.uniform_(-1.0, 3.0);
auto v = x.to_vec<double>();
double lo = 1e300, hi = -1e300, sum = 0;
for (double e : v) { lo = std::min(lo, e); hi = std::max(hi, e); sum += e; }
std::cout << "min=" << lo << "  max=" << hi << "  mean~" << sum / v.size() << "\n";
```

### Output

```text
min=-0.99998  max=2.99999  mean~0.998608
```

`min` is essentially `from`, `max` approaches but does not reach `to`, and
the mean sits near the midpoint `1.0`.

Example 3. Equal bounds give a constant fill.

```cpp
auto x = pluss::series::empty({3}, pluss::ScalarType::Float64);
x.uniform_(-2.0, -2.0);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2.0000, -2.0000, -2.0000]
```

Example 4. An integer receiver, a reversed range, and a `NaN` bound are all
rejected.

```cpp
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int32);
  i.uniform_(0.0, 1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.uniform_(3.0, 1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.uniform_(0.0, std::numeric_limits<double>::quiet_NaN());
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
uniform_: receiver must have a floating-point or complex dtype
uniform_: expected finite bounds with to >= from
uniform_: expected finite bounds with to >= from
```

For an integer buffer of random values, use [`random_`](random_.md).

Example 5. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(100);
auto a = pluss::series::empty({4}, pluss::ScalarType::Float64); a.uniform_(0.0, 10.0);
pluss::rng_seed(100);
auto b = pluss::series::empty({4}, pluss::ScalarType::Float64); b.uniform_(0.0, 10.0);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Complex Receivers

For a complex receiver, the real and imaginary parts are drawn independently
from the same `[from, to)` interval, so the samples fill a square in the
complex plane — not a line segment, and not a disk.

## Gotchas And Failure Modes

- `to` is excluded. Do not rely on ever seeing the exact upper endpoint.
- `from > to`, and non-finite bounds, are rejected. Equal bounds are *not*
  an error.
- Integral receivers are rejected rather than truncating the continuous
  samples — use [`random_`](random_.md) for integer values.
- Draws are reproducible only after `pluss::rng_seed(seed)`; each call then
  advances the stream.
- `uniform_` fills an existing shape; [`rand`](../series_factory/rand.md)
  creates one.
- Not differentiable.

## Summary

`uniform_(from = 0, to = 1)` fills the receiver in place with uniform samples
from `[from, to)`:

- half-open: `from` reachable, `to` excluded; equal bounds give a constant
  fill
- reproducible under `pluss::rng_seed(seed)`; each call advances the stream
- floating-point or complex dtype only (use [`random_`](random_.md) for
  integers); not differentiable; aliases see the write

## See Also

- [`pluss::series::rand`](../series_factory/rand.md) /
  [`pluss::series::rand_btw`](../series_factory/rand_btw.md): create a
  uniform tensor instead of filling one.
- [`Series::normal_`](normal_.md): unbounded location-scale fill.
- [`Series::random_`](random_.md): integer-valued fill.
- [`Series::exponential_`](exponential_.md): a positive, right-skewed
  continuous fill built from uniform samples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unionize_along.html">
    <span class="label">Previous</span>
    <span class="title">unionize_along</span>
  </a>
  <a class="next-link" href="view.html">
    <span class="label">Next</span>
    <span class="title">view</span>
  </a>
</div>
