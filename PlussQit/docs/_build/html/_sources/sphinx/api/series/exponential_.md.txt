<!-- pluss-hand-authored -->

# Series::exponential_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::exponential_() const
Series& Series::exponential_(double lambda) const
```

## What It Does

Overwrites every element of the receiver with an independent nonnegative
sample from an exponential distribution, whose density is
`lambda * exp(-lambda * x)` for `x >= 0`. The no-argument form uses
`lambda = 1`.

The exponential distribution models the **waiting time until the next event**
when events arrive at a constant average rate. Internally each sample is
drawn by the inverse-CDF method — `x = -ln(U) / lambda` for a uniform `U` in
`(0, 1]` — so an `exponential_` fill is a transformed
[`uniform_`](uniform_.md) fill.

Like the other in-place samplers, the receiver supplies the shape, dtype,
platform, and storage; its current values are discarded.

### `lambda` is a rate, not a scale

`lambda` is the **rate** (events per unit time). The mean and the standard
deviation are both `1 / lambda`. Doubling `lambda` *halves* the typical
waiting time. If you have a desired mean `mu`, pass `lambda = 1 / mu`.

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the values; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**lambda** (`double`, default `1`): the rate. Must be finite and `> 0`.

## Returns

`Series&`
    A reference to the receiver, now holding the samples. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if the receiver's dtype is not floating-point (complex is **not**
  accepted, unlike [`normal_`](normal_.md) / [`uniform_`](uniform_.md))
- if `lambda` is non-finite or `<= 0`

## Gradient

Not differentiable — a random overwrite carries no derivative through the
replaced values or `lambda`.

## Examples

Example 1. Default rate `1`. Every sample is nonnegative. Seeded, so the
values are fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({5}, pluss::ScalarType::Float64);
x.exponential_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.1699, 2.1792, 1.3738, 1.5186, 0.6553]
```

The distribution is right-skewed: most samples are small, with an
occasional large one.

Example 2. `lambda` is a rate — over a large sample the mean and standard
deviation are both close to `1 / lambda`.

```cpp
pluss::rng_seed(11);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.exponential_(4.0);
auto v = x.to_vec<double>();
double lo = 1e300, sum = 0;
for (double e : v) { lo = std::min(lo, e); sum += e; }
double mean = sum / v.size(), ss = 0;
for (double e : v) ss += (e - mean) * (e - mean);
std::cout << "min=" << lo << "  mean~" << mean
          << "  stddev~" << std::sqrt(ss / v.size()) << "\n";
```

### Output

```text
min=2.02657e-06  mean~0.249726  stddev~0.248912
```

`1 / 4 = 0.25` for both moments. The smallest sample is a hair above zero;
the support has no upper bound.

Example 3. `lambda <= 0`, a non-finite `lambda`, and a complex receiver are
all rejected.

```cpp
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.exponential_(0.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.exponential_(-2.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto c = pluss::series::empty({3}, pluss::ScalarType::Complex64);
  c.exponential_(1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
exponential_: expected a finite lambda > 0
exponential_: expected a finite lambda > 0
exponential_: receiver must have a floating-point dtype
```

Example 4. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(5);
auto a = pluss::series::empty({4}, pluss::ScalarType::Float64); a.exponential_(2.0);
pluss::rng_seed(5);
auto b = pluss::series::empty({4}, pluss::ScalarType::Float64); b.exponential_(2.0);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Gotchas And Failure Modes

- Pass a **rate**, not a mean. For mean `mu`, use `lambda = 1 / mu`.
- `lambda = 0` and negative rates are rejected.
- Complex receivers are rejected (a difference from
  [`normal_`](normal_.md) / [`uniform_`](uniform_.md)); the dtype must be a
  real float.
- Samples are nonnegative but have **no upper bound** — a fixed-size
  histogram or clamp may be needed downstream.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- The trailing underscore marks the in-place mutation, not a distribution
  variant.
- Not differentiable.

## Summary

`exponential_(lambda = 1)` fills the receiver in place with exponential
waiting-time samples:

- density `lambda * exp(-lambda*x)` for `x >= 0`; `lambda` is a **rate**,
  mean = stddev = `1 / lambda`
- built by inverse-CDF from a uniform draw
- real floating dtype only (no complex); not differentiable
- reproducible under `pluss::rng_seed(seed)`; aliases see the write

## See Also

- [`Series::uniform_`](uniform_.md): the underlying uniform fill that
  `exponential_` transforms.
- [`Series::geometric_`](geometric_.md): the discrete analogue — a count of
  trials until the first success.
- [`Series::log_normal_`](log_normal_.md): another positive, right-skewed
  continuous fill.
- [`pluss::series::poisson`](../series_factory/poisson.md): counts of events
  in a fixed interval, the companion of exponential inter-arrival times.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expand_as.html">
    <span class="label">Previous</span>
    <span class="title">expand_as</span>
  </a>
  <a class="next-link" href="expm1.html">
    <span class="label">Next</span>
    <span class="title">expm1</span>
  </a>
</div>
