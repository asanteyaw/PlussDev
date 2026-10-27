<!-- pluss-hand-authored -->

# Series::cauchy_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::cauchy_() const
Series& Series::cauchy_(double median) const
Series& Series::cauchy_(double median, double sigma) const
```

## What It Does

Overwrites every element of the receiver with an independent draw from a
Cauchy distribution — a symmetric, bell-*ish* curve with **much heavier
tails** than a normal. Defaults are `median = 0` and `sigma = 1`.

`median` is the center (the distribution is symmetric about it); `sigma` is
the half-width at half-maximum — larger `sigma` spreads the central mass
wider. Internally each sample is `median + sigma * tan(pi * (U - 0.5))` for a
uniform `U` — a transformed [`uniform_`](uniform_.md) fill.

The defining feature: a Cauchy distribution has **no finite mean and no
finite variance**. A running sample average does *not* settle down as you
draw more values — it keeps jumping whenever a large tail sample appears. Use
the **median** and quantiles, never the arithmetic mean, to summarize Cauchy
samples. It is a poor model choice anywhere downstream logic assumes a
finite variance.

Like the other in-place samplers, the receiver supplies the shape, dtype,
platform, and storage; its current values are discarded.

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the values; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**median** (`double`, default `0`): the center of symmetry. Must be finite.

**sigma** (`double`, default `1`): the scale (half-width at half-maximum).
Must be finite and `> 0`.

## Returns

`Series&`
    A reference to the receiver, now holding the samples. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if the receiver's dtype is not floating-point
- if `median` is non-finite, or `sigma` is non-finite or `<= 0`

## Gradient

Not differentiable.

## Examples

Example 1. Default `median = 0`, `sigma = 1`. Seeded, so fixed — note the
one large tail value.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({10}, pluss::ScalarType::Float64);
x.cauchy_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[1.8709, -2.6941, -0.9804, -1.2163, 0.0607, 0.2488, 0.8665, 1.1244, 9.4348, -1.1772]
```

Most values sit within a few units of `0`; `9.4348` is the heavy tail
showing up in a sample of just ten.

Example 2. `median` and `sigma`. Over a large sample the **mean does not
converge** to the location, but the **median does**.

```cpp
pluss::rng_seed(7);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.cauchy_(2.0, 0.5);
std::cout << "sample mean~"   << x.mean().host<double>()[0]
          << "  sample median~" << x.median().host<double>()[0]
          << "  (location = 2)\n";
```

### Output

```text
sample mean~1.90777  sample median~2.00098  (location = 2)
```

With 200,000 samples the mean is still off by ~0.09 and will keep wandering;
the median has pinned the location to three digits.

Example 3. Non-floating receiver, `sigma <= 0`, and non-finite parameters
are rejected.

```cpp
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int64);
  i.cauchy_();
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.cauchy_(0.0, 0.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.cauchy_(std::numeric_limits<double>::infinity(), 1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
cauchy_: receiver must have a floating-point dtype
cauchy_: expected finite parameters with sigma > 0
cauchy_: expected finite parameters with sigma > 0
```

Example 4. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(5);
auto a = pluss::series::empty({6}, pluss::ScalarType::Float64); a.cauchy_(0.0, 2.0);
pluss::rng_seed(5);
auto b = pluss::series::empty({6}, pluss::ScalarType::Float64); b.cauchy_(0.0, 2.0);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Gotchas And Failure Modes

- **No finite mean or variance.** Never summarize Cauchy samples with
  [`mean`](mean.md) / [`std`](std.md) — use [`median`](median.md) and
  [`quantile`](quantile.md). Do not feed them into code that assumes a
  finite variance.
- Samples are unbounded in both directions and large excursions are far more
  common than under a normal law. A downstream clamp or robust loss may be
  needed.
- Floating-point receiver only; `sigma` must be strictly positive.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`cauchy_(median = 0, sigma = 1)` fills the receiver in place with
heavy-tailed Cauchy samples:

- symmetric about `median`; `sigma` sets the half-width
- **no finite mean or variance** — summarize with the median, not the mean
- built by inverse-CDF (`tan`) from a uniform draw; floating-point receiver
  only
- reproducible under `pluss::rng_seed(seed)`; not differentiable; aliases
  see the write

## See Also

- [`Series::normal_`](normal_.md): the finite-variance location-scale
  sampler — reach for this unless you specifically want heavy tails.
- [`Series::log_normal_`](log_normal_.md): heavy *right* tail, but positive
  and with finite moments.
- [`Series::uniform_`](uniform_.md): the uniform fill `cauchy_` transforms
  internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cartprod.html">
    <span class="label">Previous</span>
    <span class="title">cartprod</span>
  </a>
  <a class="next-link" href="cbrt.html">
    <span class="label">Next</span>
    <span class="title">cbrt</span>
  </a>
</div>
